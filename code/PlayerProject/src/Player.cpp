#include "Player.h"
#include<iostream>
#include<string.h>

Player::Player(){}
Player::~Player(){stop();}

bool Player::open(const std::string& s)
{   AVFormatContext* av_fmt_ctx = nullptr;
    if(avformat_open_input(&av_fmt_ctx,s.c_str(),nullptr,nullptr)<0)
    return false;
    
    m_avformat.reset(av_fmt_ctx);


    if(avformat_find_stream_info(m_avformat.get(),nullptr)<0)
    {
        std::cout<<"未解析文件流"<<std::endl;
    }

    if(!findStreams())
    {
        std::cout<<"未找到文件流"<<std::endl;
        return false;
    }

    return true;
}

bool Player::findStreams()
{
    int audio_index=-1;
    int video_index=-1;
    for(int i=0;i<m_avformat->nb_streams;i++)
    {
        AVCodecParameters* codec_par = m_avformat->streams[i]->codecpar;
        if(codec_par->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            m_videoStreamIndex = i;
            m_videoTimeBase = m_avformat->streams[i]->time_base;
        }
        else if(codec_par->codec_type==AVMEDIA_TYPE_AUDIO)
        {
            m_audioStreamIndex = i;
            m_audioTimeBase = m_avformat->streams[i]->time_base;
        }
    }
    if(m_audioStreamIndex== -1&& m_videoStreamIndex == -1)
    return false;
return true;
}

bool Player::initDecoder()
{
    if(m_audioStreamIndex!=-1)
    {
        AVCodecParameters* audio_par = m_avformat->streams[m_audioStreamIndex]->codecpar;
        m_audioDecoder = std::make_unique<Decoder>();
        m_audioDecoder->open(audio_par);
        m_audioPlayer.init(audio_par);
    }

    if(m_videoStreamIndex!=-1)
    {
        AVCodecParameters* video_par = m_avformat->streams[m_videoStreamIndex]->codecpar;
        m_videoDecoder = std::make_unique<Decoder>();
        m_videoDecoder->open(video_par);
        m_videoRenderer.init(video_par,&m_audioPlayer);
    }

    return true;
}


bool Player::demuxLoop()
{ 
    while(m_isRunning)
    {   
        AVPacketPtr packet(av_packet_alloc());
        int ret = av_read_frame(m_avformat.get(),packet.get());

        if(ret==AVERROR_EOF)
        {
            std::cout<<"读取到文件结尾"<<std::endl;
            break;
        }
        else if(ret<0)
        {
            std::cout<<"读取文件失败"<<std::endl;
            break;
        }
        

        if(packet->stream_index == m_audioStreamIndex)
        {
            m_audioDecoder->sendPacket(packet.get());
            AVFramePtr audio_frame(av_frame_alloc());
            while(m_audioDecoder->receiveFrame(audio_frame.get()))
            {
                m_audioPlayer.pushaudioFrame(std::move(audio_frame));
                audio_frame.reset(av_frame_alloc());
            }
        }
        else if(packet->stream_index == m_videoStreamIndex)
        {
            m_videoDecoder->sendPacket(packet.get());
            AVFramePtr video_frame(av_frame_alloc());
            while(m_videoDecoder->receiveFrame(video_frame.get()))
            {
                m_videoRenderer.pushvideoFrame(std::move(video_frame));
                video_frame.reset(av_frame_alloc());
            }

        }
        //睡眠一秒防止循环跑满
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    }
    std::cout<<"解复用进程退出"<<std::endl;
    return true;
}

void Player::pause()
{
   m_audioPlayer.pause();
   m_videoRenderer.pause();
   std::cout<<"暂停播放"<<std::endl;
}

void Player::play()
{
    if(m_isRunning)
    {
        return;
    }
    m_isRunning = true;
    m_audioPlayer.play();
    m_videoRenderer.play();
    //核心解复用进程
    m_demuxThread = std::thread(&Player::demuxLoop,this);
    std::cout<<"开始播放"<<std::endl;
}
void Player::stop()
{   
    m_isRunning = false;
    //唤醒可能卡在队列里的线程
    m_audioPlayer.clearqueue();
    m_videoRenderer.clearqueue();
    //等待解复用线程安全退出
    if (m_demuxThread.joinable()) {
        m_demuxThread.join();
    }
    m_audioPlayer.stop();
    m_videoRenderer.stop();
}
