#include<iostream>
#include<SDL2/SDL.h>
#include "VideoRenderer.h"
#include "FFmpegPtr.h"
#include "ThreadSafeQueue.h"
#include "AudioPlayer.h"

    VideoRenderer::VideoRenderer()
    {
        std::cout<<"VideoRenderer构建完成"<<std::endl;
    };
    VideoRenderer::~VideoRenderer()
    {   
        stop();
        std::cout<<"VideoRenderer析构完成"<<std::endl;
    };

    

bool VideoRenderer::init(AVCodecParameters* video_codec_par,AudioPlayer* audioplayer)
    {   
        if(!video_codec_par || !audioplayer)
            return false;
        this->width = video_codec_par->width;
        this->height = video_codec_par->height;
        this->pix_format = (AVPixelFormat)video_codec_par->format;
        this->audioplayer = audioplayer;


     if(!sdl_init(width,height)) return false;
     if(!sws_init(video_codec_par)) return false;

     return true;

    }

    bool VideoRenderer::sdl_init(int width,int height)
    {   
        if(SDL_Init(SDL_INIT_VIDEO)<0)
        {
            std::cerr<<"SDL_Init失败"<<std::endl;
            return false;
        }
       
        SDL_Window* rawwindow = SDL_CreateWindow(
            "audio.player",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width,
            height,
            SDL_WINDOW_SHOWN
        );
        if(!rawwindow)
        {
            std::cerr<<"初始化窗口失败"<<std::endl;
            return false;
        };
        window.reset(rawwindow);
       
        SDL_Renderer* rawrenderer = SDL_CreateRenderer(
            window.get(),
            -1,
            SDL_RENDERER_ACCELERATED
        );
        if(!rawrenderer)
        {
            std::cerr<<"创建渲染器失败"<<std::endl;
            return false;
        };
        renderer.reset(rawrenderer);

        SDL_Texture* rawtexture = SDL_CreateTexture(
            renderer.get(),
            SDL_PIXELFORMAT_IYUV,
            SDL_TEXTUREACCESS_STREAMING,
            width,
            height
        );
        if(!rawtexture)
        {
            std::cerr<<"创建纹理失败"<<std::endl;
            return false;
        };
        texture.reset(rawtexture);
        
        
            return true;

    }


    bool VideoRenderer::sws_init(AVCodecParameters* codec_par)
    {
        yuv420p.reset(av_frame_alloc());
        yuv420p->width=width;
        yuv420p->height=height;
        yuv420p->format=AV_PIX_FMT_YUV420P;
        av_frame_get_buffer(yuv420p.get(),32);

        sws_ctx.reset(sws_getContext(
            width,
            height,
            (AVPixelFormat)codec_par->format,
            width,
            height,
            AV_PIX_FMT_YUV420P,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr
        ));
        if(!sws_ctx)
        {
            std::cout<<"创建转换器上下文失败"<<std::endl;
            return false;
        }

      
        return true;
    }

    //入队函数
    void VideoRenderer::pushvideoFrame(AVFramePtr video_frame)
    {
        videoframeQueue.push(std::move(video_frame));
    }

    //线程控制
    void VideoRenderer::pause()
       { m_isPaused = true;}

    void VideoRenderer::resume()
      {  m_isPaused = false;}

    void VideoRenderer::stop()
{
    m_isRunning = false;
    m_isPaused = true;

    videoframeQueue.clear();
    if(m_renderThread.joinable())
    {
        m_renderThread.join();
    }
    std::cout << "视频渲染已停止" << std::endl;
}
    void VideoRenderer::clearqueue()
    {
        videoframeQueue.clear();
        std::cout<<"清空视频队列成功"<<std::endl;
    }

    //渲染一帧
   void VideoRenderer::renderOneFrame()
{
    if(!m_isRunning || m_isPaused)
        return;

    AVFramePtr video_frame;
    //从队列里拿帧，如果为空直接退出
    if(!videoframeQueue.pop(video_frame))
        return;
    //音视频同步逻辑
    //算出这一帧画面在哪一秒播放
    double video_pts_seconds = 0.0;
    if(video_frame->pts != AV_NOPTS_VALUE)
    {
        video_pts_seconds = video_frame->pts * av_q2d(m_timebase);

    }
    //获取当前音频时间钟
    double audio_clock = 0.0;
    if(audioplayer)
    {
        audio_clock = audioplayer->get_clock();
    }
    //计算时间差
    double delay = video_pts_seconds - audio_clock;
    //判定
    if(delay>0)
    {
        //视频更早，等待音频
        int sleep_ms = static_cast<int>(delay * 1000);
        if(sleep_ms>0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        }
    }
    else if(delay< -0.1)
    {
    //直接丢帧
        return;
    }
      
    std::cout << "主线程拿到视频帧"
              << " width=" << video_frame->width
              << " height=" << video_frame->height
              << " format=" << video_frame->format
              << std::endl;

    int scaled_height = sws_scale(
        sws_ctx.get(),
        video_frame->data,
        video_frame->linesize,
        0,
        height,
        yuv420p->data,
        yuv420p->linesize
    );
    if(scaled_height ==0 )
    {   
        std::cerr<<"视频帧渲染失败"<<std::endl;
        return;
    }
    std::cout << "sws_scale完成,返回高度="
              << scaled_height
              << std::endl;


    if(SDL_UpdateYUVTexture(
        texture.get(),
        nullptr,
        yuv420p->data[0],
        yuv420p->linesize[0],
        yuv420p->data[1],
        yuv420p->linesize[1],
        yuv420p->data[2],
        yuv420p->linesize[2]
    ) != 0)
    {
        std::cerr << "SDL_UpdateYUVTexture失败: "
                  << SDL_GetError()
                  << std::endl;
        return;
    }

    if(SDL_RenderClear(renderer.get()) != 0)
    {
        std::cerr << "SDL_RenderClear失败: "
                  << SDL_GetError()
                  << std::endl;
        return;
    }

    if(SDL_RenderCopy(
        renderer.get(),
        texture.get(),
        nullptr,
        nullptr
    ) != 0)
    {
        std::cerr << "SDL_RenderCopy失败: "
                  << SDL_GetError()
                  << std::endl;
        return;
    }

    SDL_RenderPresent(renderer.get());

}

//新增的渲染主循环
 void VideoRenderer::renderloop()
 {
    while(m_isRunning)
    {
        if(m_isPaused)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        renderOneFrame();

    
    }
 }

 void VideoRenderer::play()
 {  
    if(m_isRunning)
    {return;}
    m_isRunning = true;
    m_isPaused = false;
    m_renderThread = std::thread(&VideoRenderer::renderloop,this);


 }




