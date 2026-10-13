#include "AudioPlayer.h"
#include<iostream>

 AudioPlayer::AudioPlayer(){};
 AudioPlayer::~AudioPlayer(){stop();};

 bool AudioPlayer::init(AVCodecParameters* audio_codec_par)
 {
    if((void AudioPlayer::sdl_init())<0)
    {
        std::cerr<<"sdl初始化失败"<<std::endl;
        return false;
    }
    if(void AudioPlayer::swr_init()<0)
    {
        std::cerr<<"swr初始化失败"<<std::endl;
        return false;
    }
    return true;
 }

 void AudioPlayer::sdl_init()
 {
    SDL_Init(SDL_INIT_AUDIO);
    sdl_audio spec{};
    spec.freq = 44100;
    spec.channel = 2;
    spec.format = AV_PIX_FMT_AYUV;
    spec.samples = out_samples;
    spec.audiocallback = audiocallback;
    spec.usedata = this;
    
    SDL_OpenAudioDevice(m_device,0);
 }

 void AudioPlayer::swr_init(AVCodecParameters* audio_codec_par)
 {  
    //定义裸指针之后智能指针接管
    SwrContext* tmp_swr = nullptr;
    AVChannel out_ch_layout;
    av_channel_layout_default(out_ch_layout,2);
    swr_ctx.reset(tmp_swr);
    ThreadSafeQueue<AVFramePtr> audioframeQueue;
 }
 
 bool AudioPlayer::handleAudio(uint8_t* stream,int len)
 {
    while(m_isPaused || audioframeQueue.empty())
    {
        SDL_memset(stream,0,len);
        return;
    }
    AVFramePtr audio_frame;
    audioframeQueue.pop(audio_frame);
    int output_samples = swr_get_out_samples(swr_ctx.get(),audio_frame->nb_samples);
    int output_size = av_samples_get_buffer_size(nullptr,m_output_channel,output_samples,AV_SAMPLE_FMT_S16,1);
    uint8_t* output_buffer[1] = {nullptr};
    output_buffer[0] = (uint8_t*)av_malloc(output_size);
    int converted_samples = swr_convert(swr_ctx.get(),output_buffer[0],output_size,
                                      (uint8_t**)audio_frame->data,audio_frame->nb_samples);

    int convertbyte = converted_samples * m_output_channel * sizeof(int16_t);                
    //拷贝到缓冲区，声卡播放
    int copy_len =(convertbyte<len)?convertbyte : len;
    SDL_memcpy(stream,output_buffer[0],copy_len);
    if(copy_len < len)
    {
        SDL_memcpy(stream+copy_len,0,len-copy_len);
    }
    audio_clock = static_cast<double> (m_playerbyte)/m_bytepersecond;
    av_free(output_buffer[0]);
    return true;
 }
 void audiocallback(uint8_t stream,int len)
 {
    AudioPlayer *audioplayer = static_cast<AudioPlayer*> usedata;
    audioplayer->handleAudio(stream,len);
 }

void AudioPlayer::play()
 {
    m_isRunning = true;
    m_isPaused = false;
 }

 void AudioPlayer::stop()
 {
    m_isRunning = false;
    m_isPaused = true;
 }

 void AudioPlayer::pause()
 {
    m_isPaused = true;
 }

void AudioPlayer::setvolume(float volume)
{
    m_volume = std::max(0.0f,std::min(static_cast<float>(volume),1.0f));
}

void AudioPlayer::clearqueue()
{
    audioframeQueue.clear();
    m_playerbyte = 0;
    audio_clock{0.0};
}

void pushauidoQueue(AVFramePtr audio_frame)
{
    audioframeQueue.push(std::move(audio_frame));
}