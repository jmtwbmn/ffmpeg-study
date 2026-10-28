#include<iostream>
#include <algorithm>
#include<vector>
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
{   if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO)<0)
    {return false;}

    
    SDL_AudioSpec spec{};
    spec.freq = 44100;
    spec.channels = 2;
    spec.format = AUDIO_S16SYS;
    spec.samples = 1024;
    spec.callback = sdlaudiocallback; //绑定回调函数
    spec.userdata = this;

    //修改:原来最后传 0,要求硬件必须完全支持 44100/2ch/S16,
    //虚拟机声卡稍有差异就直接打开失败。改成允许 SDL 调整采样率,并用 obtained 拿回真实值
    SDL_AudioSpec obtained{};
    m_device = SDL_OpenAudioDevice(nullptr,0,&spec,&obtained,SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if(m_device==0)
    {
        std::cerr<<"打开音频设备失败: "<<SDL_GetError()<<std::endl;  //修改:原来写的是"打开文件失败",且没带错误原因
        return false;
    }
    //修改:按设备实际协商出来的参数算,保证回调里的 len 和时钟的 bytepersecond 口径一致
    m_outsamplerate = obtained.freq;
    m_outchannel    = obtained.channels;
    m_bytepersecond = (double)m_outsamplerate * m_outchannel *2;
    SDL_PauseAudioDevice(m_device,0);
    return true;
}

bool AudioPlayer::swr_init(AVCodecParameters* audio_codec_par)
{   

   
     //定义一个裸指针暂时接受数据
        SwrContext* tmp_swr = nullptr;


    AVChannelLayout out_ch_layout; 
    av_channel_layout_default(&out_ch_layout,2);
    swr_alloc_set_opts2(&tmp_swr,&out_ch_layout,AV_SAMPLE_FMT_S16,m_outsamplerate,//输出
        &audio_codec_par->ch_layout,(AVSampleFormat)audio_codec_par->format,audio_codec_par->sample_rate,//输入
        0,nullptr);

         swr_ctx.reset(tmp_swr); //让m_swr_ctx用智能指针接管tmp_swr

       //这里调用的是ffmpeg的初始化，需要传入上下文
        if(::swr_init(swr_ctx.get())<0)
        {
            return false;
        }
    return true;
}

//回调函数
void AudioPlayer::sdlaudiocallback(void* userdata,Uint8* stream,int len)
{
    AudioPlayer* player = static_cast<AudioPlayer*>(userdata);
    player->handleAudio(stream,len);
}


//核心函数，不用写循环
void AudioPlayer::handleAudio(Uint8* stream,int len)
{   //先在没有数据的时候静音
    SDL_memset(stream,0,len);
    if(m_isPaused || !swr_ctx) return;   //修改:swr 没初始化好就直接静音,避免下面拿空上下文去算
    int written = 0;
    while(written < len)
    {
        if(m_pcmPos >= m_pcmBuf.size()) //当前pcm满了，取下一帧
        {
            AVFramePtr audio_frame;
            if(!audioframeQueue.try_pop(audio_frame)) break;  //没有数据了，剩下部分静音
            
            //增加，记录本帧的的流时间，作为时钟锚点
            if(audio_frame->pts != AV_NOPTS_VALUE)
            {
                m_framePts = audio_frame->pts *av_q2d(m_audioTimeBase) - m_baseTime;
            }
            m_frameBytes = 0;
            //分配缓冲区
            int output_samples = swr_get_out_samples(swr_ctx.get(),audio_frame->nb_samples);
            //新增守卫:swr_get_out_samples 出错时返回负的错误码,若直接拿去 resize
            //会被转成巨大的 size_t,vector 抛 length_error,而这是音频线程,没人接 -> 进程直接终止
            if(output_samples <= 0) {m_pcmBuf.clear(); m_pcmPos = 0; continue;}
            int output_size = av_samples_get_buffer_size(nullptr,m_outchannel,output_samples,
                                                        AV_SAMPLE_FMT_S16,0);
            //新增守卫:同上,样本数为负时这里也会算出负数,必须挡掉
            if(output_size <= 0) {m_pcmBuf.clear(); m_pcmPos = 0; continue;}
            m_pcmBuf.resize(output_size);
            uint8_t* out_planes[1] = {m_pcmBuf.data()};
            int n = swr_convert(swr_ctx.get(),out_planes,output_samples,
                                audio_frame->data,audio_frame->nb_samples);
           if(n<=0) {m_pcmBuf.clear(); m_pcmPos = 0; continue;}
            
           m_pcmBuf.resize((std::size_t)n* m_outchannel *2);    //改到真实长度
           m_pcmPos = 0;

            // 音量控制
    // 只有当音量不是 1.0（原音量）且确实转换出了数据时才处理
        if (m_volume < 1.0 ) {
        // 把 uint8_t* 的字节流，强行看作 int16_t* 数组
        // 因为 S16 格式，每个样本正好占 2 个字节
        int16_t* samples = (int16_t*)m_pcmBuf.data();
        // 循环遍历每一个样本
        // converted_samples 是“帧数”，m_outchannel 是声道数（比如 2）。
        // 一帧包含左右两个声道的数据，所以要乘以声道数，才算总的样本个数。
        for (int i = 0; i < n * m_outchannel; i++) {
        // 将原数据乘以音量系数（比如 0.5），然后强制转回 int16_t
        // 原本 -32768 到 32767 的数据，乘以 0.5 后变成 -16384 到 16383。
        // 振幅变小了，声音就变小了。
        samples[i] = (int16_t)(samples[i] * m_volume);
    }
  }
}
        int copy =std::min((int)(m_pcmBuf.size() - m_pcmPos),len - written);
        SDL_memcpy(stream + written,  m_pcmBuf.data() + m_pcmPos, copy);
        written += copy;
        m_pcmPos += copy;
        m_frameBytes += copy;   //新增:累加本帧已消费的字节,供 pts 锚定时钟使用(必须在循环内)
 }    
    //更新时钟:锚定到当前音频帧的 pts,丢帧后时钟会跟着内容一起跳
    audio_clock = m_framePts + (double)m_frameBytes / m_bytepersecond
                                -(double) len/ m_bytepersecond;
}

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
    m_isPaused = true;
    SDL_PauseAudioDevice(m_device,1);
    audioframeQueue.clear();
    audio_clock=0.0;
    std::cout<<"音频已停止播放"<<std::endl;
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
}

void AudioPlayer::set_clock(double seconds)
{
    //时钟值直接跳过去。真正的推进由 handleAudio 里的 PTS 锚定负责,
    //这里只用于"还没出第一帧音频"或 seek 之后的立即生效
    audio_clock = seconds;
}
void AudioPlayer::pushaudioFrame(AVFramePtr audio_frame)
{
    //点播更大的上限，避免内容丢帧到接近结尾
    //直播上限稍低，保持低延迟
    std::size_t cap = m_isLive ? 60:150;
   while(audioframeQueue.size()>= cap)
    {
        AVFramePtr dropped;             //丢掉最老的一帧
       
        if(!audioframeQueue.try_pop(dropped)) break;
    }
    audioframeQueue.push(std::move(audio_frame));
}


void AudioPlayer::setTimeBase(AVRational tb,double base)
{
    m_audioTimeBase = tb;
    m_baseTime = base;
}





