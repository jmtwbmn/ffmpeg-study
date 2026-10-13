#pragma once
#include<iostream>
#include "ThreadSafeQueue.h"
#include "FFmpeg.h"
#include<SDL2/SDL.h>
#include<atomic>
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

AudioPlayer(const AudioPlayer&) = delete;
AudioPlayer& operator=(const AudioPlayer&) = delete;

public:
//对外接口
    bool init(AVCodecParameters* audio_codec_par);
    void pushaudioFrame(AVFrame* audio_frame);
    void play();
    void stop();
    void pause();
    void setvolume(float volume);
    double getAudioClock() const;
    double clearqueue();

private:
//内部具体函数和参数
    void sdl_init();
    void swr_init(AVCodecParameters* audio_codec_par);
    void handleaudio(uint8_t* stream,int len);
    static void audiocallback(void* usedata,uint8_t* stream,int len);

    SwrContextPtr swr_ctx;
    SDL_AudioDeviceID m_device;
    std::atomic<bool> m_isRunning{true};
    std::atomic<bool> m_isPaused{false};
    double m_bytepersecond = 0;
    std::atomic<uint64_t>m_playerbyte = 0;
    std::atomic<double> audio_clock{0.0};
    int outputsamplerate = 44100;
    int m_channel = 2;
    double m_volume = 1;
    ThreadSafeQueue<AVFramePtr> threadaudioQueue;

};