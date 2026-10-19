#include "AudioPlayer.h"
#include<iostream>
#include<algorithm>
 AudioPlayer::AudioPlayer(){};
 AudioPlayer::~AudioPlayer(){
   stop();
   SDL_CloseAudioDevice(m_device);
};

 bool AudioPlayer::audio_init(AVCodecParameters* audio_codec_par)
 {
    if(!sdl_init())
    {
        std::cerr<<"sdl初始化失败"<<std::endl;
        return false;
    }
    if(!swr_init(audio_codec_par))
    {
        std::cerr<<"swr初始化失败"<<std::endl;
        return false;
    }
    return true;
 }

 bool AudioPlayer::sdl_init()
 {    
    SDL_Init(SDL_INIT_AUDIO);
    SDL_AudioSpec spec{};
    spec.freq = 44100; 
    spec.channels = 2;
    spec.format = AUDIO_S16SYS;
    spec.samples = 1024;
    spec.callback = audiocallback;
    spec.userdata = this;
    
    m_device = SDL_OpenAudioDevice(nullptr,0,&spec,nullptr,0);
    return true;
 }

 bool AudioPlayer::swr_init(AVCodecParameters* audio_codec_par)
 {  
    //定义裸指针之后智能指针接管
    
    SwrContext* tmp_swr = nullptr;
    AVChannelLayout out_ch_layout;
    av_channel_layout_default(&out_ch_layout,2);
    int ret = swr_alloc_set_opts2(&tmp_swr,&out_ch_layout,AV_SAMPLE_FMT_S16
      ,44100,&audio_codec_par->ch_layout,(AVSampleFormat)audio_codec_par->format,audio_codec_par->sample_rate,0,nullptr);
    swr_ctx.reset(tmp_swr);
    return true;
 }
 
 bool AudioPlayer::handleAudio(uint8_t* stream,int len)
 {
    if(m_isPaused || audioframeQueue.empty())
    {
        SDL_memset(stream,0,len);
        return false;
    }
    AVFramePtr audio_frame;
    audioframeQueue.pop(audio_frame);
    int output_samples = swr_get_out_samples(swr_ctx.get(),audio_frame->nb_samples);
    int output_size = av_samples_get_buffer_size(nullptr,2,output_samples,AV_SAMPLE_FMT_S16,1);
    uint8_t* output_buffer[1] = {nullptr};
    output_buffer[0] = (uint8_t*)av_malloc(output_size);
    int converted_samples = swr_convert(swr_ctx.get(),output_buffer,output_samples,
                                      (uint8_t**)audio_frame->data,audio_frame->nb_samples);

    int convertbyte = converted_samples * 2 * sizeof(int16_t);                
    //拷贝到缓冲区，声卡播放
    int copy_len =(convertbyte<len)?convertbyte : len;
    SDL_memcpy(stream,output_buffer[0],copy_len);
    if(copy_len < len)
    {
        SDL_memset(stream+copy_len,0,len-copy_len);
    }
    audio_clock = static_cast<double> (m_playerbyte)/m_bytepersecond;
    av_free(output_buffer[0]);
    return true;
 }
void AudioPlayer::audiocallback(void* userdata,uint8_t* stream,int len)
 {
    AudioPlayer *audioplayer = static_cast<AudioPlayer*> (userdata);
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
    audio_clock = 0.0;
}

void AudioPlayer::pushaudioFrame(AVFramePtr audio_frame)
{  
  
    audioframeQueue.push(std::move(audio_frame));
}