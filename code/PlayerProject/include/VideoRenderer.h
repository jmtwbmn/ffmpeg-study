#pragma once

extern "C"
{
#include<libavcodec/avcodec.h>
#include<libswscale/swscale.h>

}
#include<thread>
#include<SDL2/SDL.h>
#include<atomic>
#include "FFmpegPtr.h"
#include "ThreadSafeQueue.h"
#include<iostream>

class AudioPlayer;

class VideoRenderer{

    public:
    VideoRenderer() = default;
    ~VideoRenderer() = default;

    // 禁用拷贝
    VideoRenderer(const VideoRenderer&) = delete;
    VideoRenderer& operator = (const VideoRenderer&) = delete;

    //对外提供的初始化接口      --    保存音频指针为了找到音频时钟
    bool init(AVCodecParameters* codec_par, AudioPlayer* audioPlayer);

    //对外提供入队接口
    void pushFrame(AVFramePtr frame);

    //调用函数
    void play();
    void pause();
    void resume();
    void stop();

    private:
    //核心函数
    bool sws_init(AVCodecParametersPtr codec_par);
    bool sdl_init(int width,int height);
    void renderloop();

    //队列
    ThreadSafeQueue<AVFramePtr> videoframeQueue;

  
    //成员变量
    SDL_WindowPtr window;
    SDL_RendererPtr renderer;
    SDL_TexturePtr texture;
    SwsContextPtr sws_ctx;
    AVFramePtr yuv420p;
    int width=0;
    int height=0;
    AVPixelFormat pix_foramt = AV_PIX_FMT_NONE;

    private:
    //线程状态
    std::thread renderThread;
    std::atomic<bool> m_isRunning{false};   //通过{}列表初始化，避免赋值拷贝更稳妥
    std::atomic<bool> m_isPaused{false};


    private:
    //音视频同步,用于获取时钟
    AudioPlayer* audioplayer = nullptr;


};