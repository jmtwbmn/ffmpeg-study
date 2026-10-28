#pragma once
#include<SDL2/SDL.h>
#include<iostream>
#include<atomic>
#include<vector>   //新增:下面用到了 std::vector,原来靠 <queue> 间接引入,不可靠
#include "FFmpegPtr.h"
#include "ThreadSafeQueue.h"

extern "C"
{
#include<libavcodec/avcodec.h>
#include<libswscale/swscale.h>
#include<libswresample/swresample.h>
}

class AudioPlayer{

    public:
    AudioPlayer();
    ~AudioPlayer();

    //禁用拷贝
    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator = (const AudioPlayer&) = delete;

    //对外统一初始化接口
    bool init(AVCodecParameters* audio_codec_par);
    //控制
    public:
    void pause();
    void play();
    void clearqueue();
    void set_clock(double seconds);
    void stop();
    void setvolume(float volume);
    double get_clock() const;
    void pushaudioFrame(AVFramePtr audio_frame);
    std::atomic<double> audio_clock {0.0};
    
    void setTimeBase(AVRational tb,double base);
    void setLive(bool live){m_isLive = live;}
    //参数
    private:
    bool swr_init(AVCodecParameters* audio_codec_par);
    bool sdl_init();
    std::atomic<bool> m_isPaused{true};

    ThreadSafeQueue<AVFramePtr> audioframeQueue;
    SDL_AudioDeviceID m_device = 0;
    SwrContextPtr swr_ctx;
    int m_outsamplerate = 44100;
    int m_outchannel = 2;
    double m_volume = 1;
    //拉流时不能用点播的固定参数了
    std::vector<uint8_t> m_pcmBuf;   //当前帧转好的pcm
    std::size_t m_pcmPos = 0;       //这个缓冲区用掉的字节数

    //音频时钟:在音频线程的回调函数写入,在视频线程读取用于同步
    double m_bytepersecond = 0;                      //每秒传输的字节数(计算时间)

    //核心函数
   
    //处理函数
    void handleAudio(Uint8* stream, int len); 
    //handleaudio调用，底层返回的是静态的回调函数，定义类型会报错，遇到错误时应该静音然后退出

    //回调函数
    static void sdlaudiocallback(void* userdata,Uint8* stream,int len);

    //时钟锚点定到音频帧pts
    AVRational m_audioTimeBase{1,1000};
    double m_baseTime = 0.0;
    double m_framePts = 0.0;    //当前帧在流里的秒数
    uint64_t m_frameBytes =0;   //本帧消费的字节数

    bool m_isLive = false;
};