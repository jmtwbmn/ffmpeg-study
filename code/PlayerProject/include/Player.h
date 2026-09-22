#pragma once
#include "AudioPlayer.h"
#include "Decoder.h"
#include "FFmpegPtr.h"
#include "ThreadSafeQueue.h"
#include "VideoRenderer.h"
#include<iostream>

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

   
private:
//2.内部函数
    bool findStreams();         //找文件流
    bool initDecoder();         //初始化解码器
    bool demuxLoop();           //核心主循环

private:
//3.关键声明变量
    AVFormatContextPtr m_avformat;      //文件上下文
    AudioPlayer m_audioPlayer;
    VideoRenderer m_videoRenderer;                  //关键工具
    std::unique_ptr<Decoder> m_audioDecoder;
    std::unique_ptr<Decoder> m_videoDecoder;

    ThreadSafeQueue<AVPacketPtr> m_audiopacketqueue;
    ThreadSafeQueue<AVPacketPtr> m_videopacketqueue;

    //线程和状态控制
    std::thread m_demuxThread;
    std::atomic<bool> m_isRunning {false};

    //记录流信息和时间基
    int m_audioStreamIndex = -1;
    int m_videoStreamIndex = -1;
    AVRational m_audioTimeBase;
    AVRational m_videoTimeBase;


    

};