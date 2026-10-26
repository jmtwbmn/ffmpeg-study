#include<iostream>
#include<SDL2/SDL.h>
#include "VideoRenderer.h"
#include "FFmpegPtr.h"
#include "ThreadSafeQueue.h"
#include "AudioPlayer.h"
#include "Player.h"

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
        //SDL在audioPlayer中已经初始化了，重复初始化会出错
       
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
    {   //丢帧逻辑,防止视频队列过快
        while(videoframeQueue.size() >= m_maxQueueFrames)
        {
            AVFramePtr dropped;
            if(!videoframeQueue.try_pop(dropped)) break;
        }
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
    m_pendingFrame.reset();
   
    std::cout << "视频渲染已停止" << std::endl;
}
    void VideoRenderer::clearqueue()
    {
        videoframeQueue.clear();
        m_pendingFrame.reset();
        std::cout<<"清空视频队列成功"<<std::endl;
    }

    //渲染一帧
   void VideoRenderer::renderOneFrame()
{
    if(!m_isRunning || m_isPaused)
        return;
 
    //优化，每次队列先看后取，音频钟到了再取出，避免队列堵塞丢帧
    int64_t frontPts = AV_NOPTS_VALUE;
    if(!videoframeQueue.peek([&](AVFramePtr& f){ frontPts = f->pts;}))
    return;

    double video_pts_seconds = (frontPts != AV_NOPTS_VALUE)
            ? frontPts * av_q2d(m_timebase) - m_baseTime : 0.0;

    //获取当前音频时间钟
    double audio_clock = 0.0;
    if(audioplayer)
    {
        audio_clock = audioplayer->get_clock();
    }
    //没有音频流或者音频钟没起来时,退化成系统时钟
    if(!audioplayer || audio_clock <= 0.0)
    {   
        if(m_startTicks == 0) m_startTicks = SDL_GetTicks64();
        audio_clock = (SDL_GetTicks64() - m_startTicks) /1000.0;
    }
    double delay = video_pts_seconds - audio_clock;

    if(delay>0.005) return;   //提前了，队列不动
    if(!videoframeQueue.try_pop(m_pendingFrame)) return;//到点了才取出
    if(delay<-0.15)           //延迟太多了，丢帧
    {
        m_pendingFrame.reset();
        return;
    }
      
   
    int scaled_height = sws_scale(
        sws_ctx.get(),
        m_pendingFrame->data,
        m_pendingFrame->linesize,
        0,
        height,
        yuv420p->data,
        yuv420p->linesize
    );
    if(scaled_height <0 )
    {   
        std::cerr<<"视频帧渲染失败"<<std::endl;
        return;
    }
  
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
    m_frameDrawn = true;
    m_pendingFrame.reset();
   // SDL_RenderPresent(renderer.get());

  
}
/*
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
*/
 void VideoRenderer::play()
 {  
    if(m_isRunning)
    {return;}
    m_isRunning = true;
    m_isPaused = false;
    m_startTicks = SDL_GetTicks64(); //记下起点，now-起点就是已播秒数
   
 }


