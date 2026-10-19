
#include<VideoPlayer.h>
#include<iostream>
#include<SDL2/SDL.h>

VideoPlayer::VideoPlayer(){}
VideoPlayer::~VideoPlayer(){stop();}

bool VideoPlayer::video_init(AVCodecParameters* video_codec_par)
{   if(!video_codec_par)
    {
        return false;
    }
    //保存视频信息到变量
    width = video_codec_par->width;
    height = video_codec_par->height;
    pix_format = (AVPixelFormat)video_codec_par->format;
    if(!sdl_init())
    {
        std::cerr<<"sdl初始化失败"<<std::endl;
    }
    if(!sws_init(AVCodecParameters* video_codec_par))
    {
        std::cerr<<"sws初始化失败"<<std::endl;
    }
    return true;
}

bool VideoPlayer::sdl_init()
{   
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr<<"video_sdl初始化失败"<<std::endl;
    }
    
    //顺序:标题,x,y,宽，高，标志
    m_window.reset(SDL_CreateWindow(
        "av_player",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
         width,
         height,
         SDL_WINDOW_SHOWN
    ));
    if(!m_window)
    {   
        std::cerr<<"window初始化失败"<<std::endl;
        return false;
    }
    
    m_renderer.reset(SDL_CreateRenderer(
        m_window.get(),
        -1,
        SDL_RENDERER_ACCELERATED
        ));
    if(!m_renderer)
    {
        std::cerr<<"renderer初始化失败"<<std::endl;
        return false;
    }

    m_texture.reset(SDL_CreateTexture(
        m_renderer.get(),
        SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    ));
    if(!m_texture)
    {
        std::cerr<<"texture初始化失败"<<std::endl;
        return false;
    }
    return true;
}
    bool VideoPlayer::sws_init(AVCodecParameters* video_codec_par)
    {
        //sws_getcontext自动初始化和释放资源
        //输入和输出格式
        sws_ctx.reset(sws_getContext(
            width,height,pix_format,
            width,height,AV_PIX_FMT_YUV420P,
            SWS_BILINEAR,nullptr,nullptr,nullptr
        ));
    if(!sws_ctx)
    {
        return false;
    }
    yuv420p.reset(av_frame_alloc());
    yuv420p->width = width;
    yuv420p->height = height;
    yuv420p->format = AV_PIX_FMT_YUV420P;
    if(av_frame_get_buffer(yuv420p.get(),32)<0)
    {
        std::cerr<<"yuv申请空间失败"<<std::endl;
        return false;
    }
    return true;
    }
    //入队接口
    void VideoPlayer::pushvideoQueue(AVFramePtr video_frame)
    {
        videoframeQueue.push(std::move(video_frame));
    }

    void VideoPlayer::renderoneframe()
    {   
        if(!m_isRunning ||m_isPaused)
        return;
        AVFramePtr video_frame;
        if(!videoframeQueue.pop(video_frame))
        {
            return;
        }
       
        int scale_output = sws_scale(
                sws_ctx.get(),
                video_frame->data,
                video_frame->linesize,
                0,
                height,
                yuv420p->data,
                yuv420p->linesize
            );
        if(scale_output ==0 )
        {
            std::cerr<<"视频帧渲染失败"<<std::endl;
        }
        if(SDL_UpdateTexture(
            m_texture.get(),
            nullptr,
            yuv420p->data[0],yuv420p->linesize[0],
            yuv420p->data[1],yuv420p->linesize[1],
            yuv420p->data[2],yuv420p->linesize[2]
        )!=0){
        std::cerr<<"SDL_UpdateYUVTexture失败"<<SDL_GetError()<<std::endl;
        return;}
     //三部曲
        if(SDL_RenderClear(m_renderer.get())!=0)
        {
            std::cerr<<"SDL_RenderCopy失败"<<SDL_GetError()<<std::endl;
            return;
        }
        if(SDL_RenderCopy(m_renderer.get(),m_texture.get(),nullptr,nullptr)!=0)
        {
            std::cerr<<"SDL_RenderCopy失败"<<SDL_GetError()<<std::endl;
            return;
        }
        SDL_RenderPresent(m_renderer.get());
    }
}

//主线程渲染循环
void VideoPlayer::renderloop()
{
   while(m_isRunning)
   {
    if(m_isPaused)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
    }
    renderoneloop();
    //帧率控制，后续在这里进行audio_clock音视频同步
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
   }


}
    void VideoPlayer::play()
    {   if(m_isRunning)
        return ;
        m_isRunning = true;
        m_isPaused = false;
        //启动主线程循环
        m_renderThread = std::thread(&VideoPlayer::renderloop,this);
        std::cout<<"videoplayer开始播放"<<std::endl;
    };
    void VideoPlayer::pause()
    {
        m_isPaused = true;
        std::cout<<"videoplayer已暂停"<<std::endl;
    };
    void VideoPlayer::stop()
    {   if(!m_isRunning) return;
        m_isRunning = false;
        m_isPaused = true;
        //唤醒可能卡在队列里的渲染线程  
        videoframeQueue.clear();
        if(m_renderThread.joinable())
        {
            m_renderThread.join();
        }
        std::cout<<"videoplayer已停止"<<std::endl;
    }
    void VideoPlayer::clear()
    {
        videoframeQueue.clear();
    }


