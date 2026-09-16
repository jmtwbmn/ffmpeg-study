#pragma once
#include<SDL2/SDL.h>
#include<iostream>
#include<atomic>
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
    void init(AVCodecParameters* audio_codec_par);
    //控制
    public:
    void pause();
    void play();
    void resume();
    void clearqueue();
    void setvolume(double volume);
    double get_clock() const;

    //参数
    private:
    bool swr_init(AVCodecParameters* audio_codec_par);
    bool sdl_init();
    bool pushaudioFrame(AVFramePtr audio_frame);

    ThreadSafeQueue<AVFramePtr> audioframeQueue;
    SDL_AudioDeviceID m_device = 0;
    SwrContextPtr swr_ctx;
    int m_outsamplerate = 44100;
    int m_outchannel = 2;
    double m_volume = 1;

    //音频时钟“音频同步核心”    在音频线程的回调函数写入更新，在视频线程读取用于同步
    std::atomic<double> audio_clock{0.0};            //当前播放到第几秒
    std::atomic<uint64_t> m_playerbyte = 0;          //已经输送了多少字节
    double m_bytepersecond = 0;                      //每秒传输的字节数(计算时间)

    //核心函数
   
    //处理函数
    void handleAudio(Uint8* stream, int len); 
    //handleaudio调用，底层返回的是静态的回调函数，定义类型会报错，遇到错误时应该静音然后退出

    //回调函数
    static void sdlaudiocallback(void* userdata,Uint8* stream,int len);

};