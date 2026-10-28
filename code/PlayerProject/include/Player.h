#pragma once
#include "AudioPlayer.h"
#include "Decoder.h"
#include "FFmpegPtr.h"
#include "ThreadSafeQueue.h"
#include "VideoRenderer.h"
#include<iostream>
#include<mutex>

extern "C"
{
#include<libavcodec/avcodec.h>
}

class Player
{

public:
    Player();
    ~Player();

//1.提供对外接口
    bool open(const std::string& file);
    void play();
    void pause();
    void stop();
    void resume();
        //状态控制
    bool getPauseState() const;

    void renderVideoFrame();
    bool takeFrameDrawn();      //本轮是否真的画了新帧,主循环据此决定要不要 Present

    bool findStreams();         //找文件流
    bool initDecoder();         //初始化解码器
    bool demuxLoop();           //核心主循环

    //额外控制，进度条功能
    double getCurrentTime() const;
    double getDuration() const;
    bool seek(double time);

    //获取窗口渲染器，用于绘制进度条
    SDL_Renderer* getRenderer();
    int getWindowWidth() const;
    int getWindowHeight() const;

private:
//3.关键声明变量
    AVFormatContextPtr m_avformat;      //文件上下文
    AudioPlayer m_audioPlayer;
    VideoRenderer m_videoRenderer;                  //关键工具
    std::unique_ptr<Decoder> m_audioDecoder;
    std::unique_ptr<Decoder> m_videoDecoder;

    //线程和状态控制
    std::thread m_demuxThread;
    std::atomic<bool> m_isRunning {false};
    std::atomic<bool> m_isPaused {true};
    std::mutex m_seekMutex;
    std::atomic<bool> m_isSeeking{false};

    //记录流信息和时间基
    int m_audioStreamIndex = -1;
    int m_videoStreamIndex = -1;
    AVRational m_audioTimeBase;
    AVRational m_videoTimeBase;

    //时间基准归一化:把"流里的绝对时间"翻译成"播放器时间轴"
    double m_baseTime = 0.0;
    bool   m_baseInited = false;

    //直播流标记(duration 无效 = 直播),用来禁掉 seek
    bool m_isLive = false;

    double m_duration = 0.0; //视频总时长；
    

};