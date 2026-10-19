#include "Player.h"
#include<iostream>
#include<chrono>
#include<thread>

Player::Player(){}
Player::~Player(){stop();}

bool Player::file_deal(const std::string& file)
{       
    AVFormatContext* tmp_ctx = nullptr;
    //打开文件
    if(avformat_open_input(&tmp_ctx,file.c_str(),nullptr,nullptr)<0)
    {
        std::cerr<<"无法打开文件"<<std::endl;
        return false;
    }
    av_fmt_ctx.reset(tmp_ctx);
    //查找流信息
    if(!findstream())
    return false;

    if(!init_decoder())
    {
        return false;
    }
    return true;
}

bool Player::findstream()
{
    
    for(unsigned int a=0;a<av_fmt_ctx->nb_streams;a++)
    {   
        AVCodecParameters* codec_par = av_fmt_ctx->streams[a]->codecpar;
        if(av_fmt_ctx->streams[a]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_index = a;
            video_timebase = av_fmt_ctx->streams[a]->time_base;
        }
        if(av_fmt_ctx->streams[a]->codecpar->codec_type== AVMEDIA_TYPE_AUDIO)
        {
            audio_stream_index = a;
            audio_timebase = av_fmt_ctx->streams[a]->time_base;
        }
    }
    //只要解析到一个流就行
    if(audio_stream_index == -1&& video_stream_index == -1) return false;
    return true;
}

bool Player::init_decoder()
{
    //初始化音频
    if(audio_stream_index != -1)
    {
        AVCodecParameters* audio_codec_par = av_fmt_ctx->streams[audio_stream_index]->codecpar;
        audio_decoder = std::make_unique<Decoder>();
        //调用decoder的open和init
        if(!audio_decoder->open(audio_codec_par)) return false;
        if(!m_audioplayer.init(audio_codec_par)) return false;
    }
    //初始化视频
    if(video_stream_index != -1)
    {
        AVCodecParameters* video_codec_par = av_fmt_ctx->streams[video_stream_index]->codecpar;
        video_decoder = std::make_unique<Decoder>();
        //调用decoder的init和open
        if(!video_decoder->open(video_codec_par)) return false;
        if(!m_videoplayer.init(video_codec_par,&m_audioplayer)) return false;
    }
    return true;
}

void Player::demuxloop()
{   
    std::cout<<"解复用线程启动"<<std::endl;
    AVPacketPtr packet(av_packet_alloc());

    while(m_isRunning)
    {
        if(m_isPaused ){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
     }
     
     int ret = av_read_frame(av_fmt_ctx.get(),packet.get());
     if(ret == AVERROR_EOF)
     {
        std::cout<<"播放完毕"<<std::endl;
        break;
     }
     else if(ret<0)
     {
        break;
     }
    //处理音频包
    if(packet->stream_index == audio_stream_index)
    {
        audio_decoder->sendPacket(packet.get());
        AVFramePtr frame(av_frame_alloc());
        while(audio_decoder->receiveFrame(frame.get()))
        {
            m_audioplayer.pushaudioFrame(std::move(frame));
            frame.reset(av_frame_alloc());
        }
    }
    //处理视频包
    if(packet->stream_index == video_stream_index)
    {
        video_decoder->sendPacket(packet.get());
        AVFramePtr frame(av_frame_alloc());
        while(video_decoder->receiveFrame(frame.get()))
        {
            m_videoplayer.pushvideoQueue(std::move(frame));
            frame.reset(av_frame_alloc());
        }
    }
    av_packet_unref(packet.get());
    }

}

void Player::play()
{
    if(m_isRunning)
    return;
    m_isRunning = true;
    m_isPaused = false;
    m_videoplayer.play();
    m_audioplayer.play();
    demuxThread = std::thread(&Player::demuxloop,this);
    std::cout<<"解复用线程开始"<<std::endl;
}

void Player::pause()
{
    if(!m_isRunning)
    return;
    m_isPaused = true;
    m_videoplayer.pause();
    m_audioplayer.pause();
}

void Player::stop()
{   
    if(!m_isRunning) return;
    m_isRunning = false;
    m_isPaused = true;
    if(demuxThread.joinable())
    {   //等待线程退出
        demuxThread.join();
    }
    m_videoplayer.stop();
    m_audioplayer.stop();
    
}