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
#include<thread>

class AudioPlayer;

class VideoRenderer{

    public:
    VideoRenderer();
    ~VideoRenderer();

    // 禁用拷贝
    VideoRenderer(const VideoRenderer&) = delete;
    VideoRenderer& operator = (const VideoRenderer&) = delete;

    //对外提供的初始化接口      --    保存音频指针为了找到音频时钟
    bool init(AVCodecParameters* video_codec_par, AudioPlayer* audioPlayer);

    //对外提供入队接口
    void pushvideoFrame(AVFramePtr video_frame);
    void clearqueue();
    void renderOneFrame();
    //调用函数
    void play();
    void pause();
    void resume();
    void stop();
    
    double m_baseTime = 0.0;
    Uint64 m_startTicks = 0;    //系统时钟起点，无音频时兜底
    AVRational m_timebase{1,100000};

    //窗口信息
    SDL_Renderer* getRenderer(){return renderer.get();}
    int getWidth() const{return width;}
    int getHeight() const{return height;}

    //实际拉流减少延迟
    std::size_t queueSize() const {return videoframeQueue.size();}

    //用exchange取走并清零，一次标记对应一次present
    bool takeFrameDrawn(){return m_frameDrawn.exchange(false);}

    //队列深度按"时间"设定,由 Player 按帧率换算成帧数后传进来
    void setQueueFrames(std::size_t n){m_maxQueueFrames = n;}

    //直播点播标记，点播堆积，直播丢帧
    void setLive(bool live){m_isLive = live;}
    std::size_t queueLimit() const {return m_maxQueueFrames;}

    private:
    //核心函数
    bool sws_init(AVCodecParameters* codec_par);
    bool sdl_init(int width,int height);
    //void renderloop();

    //队列
    ThreadSafeQueue<AVFramePtr> videoframeQueue;

  
    //成员变量
    SDL_WindowPtr window;
    SDL_RendererPtr renderer;
    SDL_TexturePtr texture;
    SwsContextPtr sws_ctx;
    AVFramePtr yuv420p;
    AVFramePtr m_pendingFrame;      //已取出但还没到显示时间的帧
    int width=0;
    int height=0;
    AVPixelFormat pix_format = AV_PIX_FMT_NONE;
    private:
    //线程状态
    std::atomic<bool> m_isRunning{false};   //通过{}列表初始化，避免赋值拷贝更稳妥
    std::atomic<bool> m_isPaused{true};


    private:
    //音视频同步,用于获取时钟
    AudioPlayer* audioplayer = nullptr;

    //用于实时控制帧率
    std::atomic<bool> m_frameDrawn{false};
    std::size_t m_maxQueueFrames = 60;
   
    bool m_isLive = false;
};