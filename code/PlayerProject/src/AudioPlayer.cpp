#include<iostream>
#include <algorithm>
#include "AudioPlayer.h"
#include "FFmpegPtr.h"
#include "ThreadSafeQueue.h"


 AudioPlayer::AudioPlayer(){std::cout<<"构造audioplayer"<<std::endl;};
 AudioPlayer::~AudioPlayer()
 {  if(m_device>0)
    {
        SDL_CloseAudioDevice(m_device);
    }
    std::cout<<"析构audioplayer 已关闭音频设备"<<std::endl;
};


bool AudioPlayer::init(AVCodecParameters* audio_codec_par)
{   
    if(!audio_codec_par)
    {
        return false;
    }
    if(!sdl_init())
    {
        std::cout<<"audio_sdl初始化失败"<<std::endl;
        return false;
    }
    
    if(! swr_init(audio_codec_par))
    {
        std::cout<<"音频上下文初始化失败"<<std::endl;
        return false;
    }
    return true;
}

bool AudioPlayer::sdl_init()
{   if(SDL_Init(SDL_INIT_AUDIO)<0)
    {return false;}

    SDL_AudioDeviceID m_device;
    SDL_AudioSpec spec{};
    spec.freq = 44100;
    spec.channels = 2;
    spec.format = AUDIO_S16SYS;
    spec.samples = 1024;
    spec.callback = sdlaudiocallback; //绑定回调函数
    spec.userdata = this;
    m_device = SDL_OpenAudioDevice(nullptr,0,&spec,nullptr,0);
    m_bytepersecond = m_outsamplerate * m_outchannel *2;
    SDL_PauseAudioDevice(m_device,0);
    return true;
}

bool AudioPlayer::swr_init(AVCodecParameters* audio_codec_par)
{   

    SwrContextPtr m_swr_ctx = nullptr;

     //定义一个裸指针暂时接受数据
        SwrContext* tmp_swr = nullptr;


    AVChannelLayout out_ch_layout; 
    av_channel_layout_default(&out_ch_layout,2);
    swr_alloc_set_opts2(&tmp_swr,&out_ch_layout,AV_SAMPLE_FMT_S16,m_outsamplerate,//输出
        &audio_codec_par->ch_layout,(AVSampleFormat)audio_codec_par->format,audio_codec_par->sample_rate,//输入
        0,nullptr);

         m_swr_ctx.reset(tmp_swr); //让m_swr_ctx用智能指针接管tmp_swr

       //这里调用的是ffmpeg的初始化，需要传入上下文
        if(::swr_init(m_swr_ctx.get())<0)
        {
            return false;
        }
    return true;
}

//回调函数
void AudioPlayer::sdlaudiocallback(void* userdata,Uint8* stream,int len)
{
    AudioPlayer* player = static_cast<AudioPlayer*>(userdata);
}


//核心函数，不用写循环
void AudioPlayer::handleAudio(Uint8* stream,int len)
{
   //暂停或者队列空的时候静音
   while(m_isPaused || audioframeQueue.empty())
   {
       SDL_memset(stream,0,len);
       return;
   }
   
   //压入一帧数据
    AVFramePtr audio_frame;
    audioframeQueue.pop(audio_frame);
   
    //分配缓冲区
    int output_samples = swr_get_out_samples(swr_ctx.get(),audio_frame->nb_samples);
    int output_size = av_samples_get_buffer_size(nullptr,m_outchannel,output_samples,AV_SAMPLE_FMT_S16,0);
    uint8_t* out_buffer[] = {nullptr};
    out_buffer[1] = (uint8_t*)av_malloc(output_size);
    //重采样，修正参数
    int converted_samples =swr_convert(swr_ctx.get(),out_buffer, output_size,(uint8_t**)audio_frame->data,audio_frame->nb_samples);
    SDL_QueueAudio(m_device,&output_samples,output_size);

    int convertbytes = converted_samples * m_outchannel * 2;

    // 音量控制
    // 只有当音量不是 1.0（原音量）且确实转换出了数据时才处理
if (m_volume < 1.0 && converted_samples > 0) {
    // 把 uint8_t* 的字节流，强行看作 int16_t* 数组
    // 因为 S16 格式，每个样本正好占 2 个字节
    int16_t* samples = (int16_t*)out_buffer[0];
    // 循环遍历每一个样本
    // converted_samples 是“帧数”，m_outchannel 是声道数（比如 2）。
    // 一帧包含左右两个声道的数据，所以要乘以声道数，才算总的样本个数。
    for (int i = 0; i < converted_samples * m_outchannel; i++) {
        // 将原数据乘以音量系数（比如 0.5），然后强制转回 int16_t
        // 原本 -32768 到 32767 的数据，乘以 0.5 后变成 -16384 到 16383。
        // 振幅变小了，声音就变小了。
        samples[i] = (int16_t)(samples[i] * m_volume);
    }
}


    // 6. 拷入 SDL 缓冲区,然后声卡调用播放
    int copy_len = (convertbytes < len) ? convertbytes : len;
    SDL_memcpy(stream, out_buffer[0], copy_len);
    if (copy_len < len) {
        SDL_memset(stream + copy_len, 0, len - copy_len);
    }



    //更新时钟
    m_playerbyte+= copy_len;
    audio_clock = static_cast<double>(m_playerbyte) / m_bytepersecond;


    av_free(out_buffer[1]);

};

void AudioPlayer::play()
{
    if(m_device>0)
    {
        m_isPaused = false;
        SDL_PauseAudioDevice(m_device,0);
    }
}

void AudioPlayer::pause()
{
    if(m_device>0)
    {
        m_isPaused = true;
        SDL_PauseAudioDevice(m_device,1);
    }
}

void AudioPlayer::stop()
{
    m_isRunning = false;
    m_isPaused = true;
    SDL_PauseAudioDevice(m_device,1);
    audioframeQueue.clear();
    m_playerbyte=0;
    audio_clock=0.0;
    std::cout<<"音频已停止播放"<<std::endl;
}


void AudioPlayer::resume()
{
    void play();
}

void AudioPlayer::setvolume(float volume)
{
    m_volume = std::max({0.0f,std::min(static_cast<float>(volume),1.0f)});    //限制音量在0～1之间
}

double AudioPlayer::get_clock() const
{
    return audio_clock.load();  //返回当前时钟
}

void AudioPlayer::clearqueue()
{
    audioframeQueue.clear();
    m_bytepersecond = 0;
    audio_clock = {0.0};

}

void AudioPlayer::pushaudioFrame(AVFramePtr audio_frame)
{
   
    audioframeQueue.push(std::move(audio_frame));
}








