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

        this->height = video_codec_par->height;
        this->pix_format = (AVPixelFormat)video_codec_par->format;
        this->audioplayer = audioplayer;


     if(!sdl_init(width,height)) return false;
     if(!sws_init(video_codec_par)) return false;

     return true;

    }

    bool VideoRenderer::sdl_init(int width,int height)
    {   

       
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
    void VideoRenderer::play()
    {
        if(m_isRunning)
        return;
        m_isPaused = false;
        m_isRunning = true;
        renderThread = std::thread(&VideoRenderer::renderloop,this);
    }
    void VideoRenderer::pause()
       { m_isPaused = true;}

    void VideoRenderer::resume()
      {  m_isPaused = false;}

    void VideoRenderer::stop()
    {   //m_isRunning和m_isPaused,前者是控制线程的生死，后者线程存在只是暂停
        if(!m_isRunning)
        return;
        m_isRunning = false;
        m_isPaused = true;
        if(renderThread.joinable())
        renderThread.join();
    }

    void VideoRenderer::clearqueue()
    {
        videoframeQueue.clear();
        std::cout<<"清空视频队列成功"<<std::endl;
    }

    //主循环,渲染
    bool VideoRenderer::renderloop()
    {
        while(m_isRunning)
        {
            if(m_isPaused)
            {   //在暂停的时候睡眠10秒防止减少cpu占用
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
                continue;
            }
        

        AVFramePtr audio_frame;
        if(!videoframeQueue.pop(video_frame))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }



        sws_scale(
            sws_ctx.get(),
            video_frame->data,
            video_frame->linesize,
            0,
            height,
            yuv420p->data,
            yuv420p->linesize

        );
        
            SDL_UpdateYUVTexture(
                texture.get(),
                nullptr,
                yuv420p->data[0],
                yuv420p->linesize[0],
                yuv420p->data[1],
                yuv420p->linesize[1],
                yuv420p->data[2],
                yuv420p->linesize[2]
            );
            //三部曲
            SDL_RenderClear(renderer.get());
            SDL_RenderCopy(renderer.get(),texture.get(),nullptr,nullptr);
            SDL_RenderPresent(renderer.get());


            //同步
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }

 

    




