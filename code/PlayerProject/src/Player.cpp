#include "Player.h"
#include "VideoRenderer.h"
#include "AudioPlayer.h"
#include<iostream>
#include<string.h>

Player::Player(){}
Player::~Player(){stop();}

bool Player::open(const std::string& s)
{   
    AVFormatContext* av_fmt_ctx = nullptr;
    if(avformat_open_input(&av_fmt_ctx,s.c_str(),nullptr,nullptr)<0)
    return false;
    
    m_avformat.reset(av_fmt_ctx);


    if(avformat_find_stream_info(m_avformat.get(),nullptr)<0)
    {
        std::cout<<"未解析文件流"<<std::endl;
        return false;
    }
    //获取文件时长
    //直播流 duration 是 AV_NOPTS_VALUE,不赋值的话 m_duration 恒为 0,
    //seek() 里会把任何时间都夹紧到 0,等于点一下进度条就跳回开头
    m_isLive = (m_avformat->duration == AV_NOPTS_VALUE || m_avformat->duration <= 0);
    if(!m_isLive)
    {
        m_duration = m_avformat->duration/(double)AV_TIME_BASE;
        std::cout<<"视频时长是 "<<m_duration<<" 秒"<<std::endl;
    }
    else
    {
        std::cout<<"直播流:无时长信息,已禁用 seek"<<std::endl;
    }
    if(!findStreams())
    {
        std::cout<<"未找到文件流"<<std::endl;
        return false;
    }
    if(!initDecoder())
    {
        std::cout<<"初始化解码器失败"<<std::endl;
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
{   m_videoRenderer.m_timebase = m_videoTimeBase;
    m_audioPlayer.setLive(m_isLive);
    m_videoRenderer.setLive(m_isLive);
    if(m_audioStreamIndex!=-1)
    {
        AVCodecParameters* audio_par = m_avformat->streams[m_audioStreamIndex]->codecpar;
        m_audioDecoder = std::make_unique<Decoder>();
        if(!m_audioDecoder->open(audio_par))
        {
            std::cerr<<"音频解码器打开失败"<<std::endl;
            return false;
        }
        if(!m_audioPlayer.init(audio_par))
        {
            std::cerr<<"音频输出初始化失败"<<std::endl;
            return false;
        }
    }

    if(m_videoStreamIndex!=-1)
    {
        AVCodecParameters* video_par = m_avformat->streams[m_videoStreamIndex]->codecpar;
        m_videoDecoder = std::make_unique<Decoder>();
        if(!m_videoDecoder->open(video_par))
        {
            std::cerr<<"视频解码器打开失败"<<std::endl;
            return false;
        }

        //队列深度按"时间"算(2秒),再按实际帧率换算成帧数。
        //固定帧数在 60fps 下只有 0.8 秒,装不下预读量,会导致持续丢帧、帧率塌到 1~2
        AVRational fr = av_guess_frame_rate(m_avformat.get(),
                                            m_avformat->streams[m_videoStreamIndex],nullptr);
        double fps = (fr.num>0 && fr.den>0) ? av_q2d(fr) : 30.0;
        std::size_t cap = (std::size_t)(fps * 2.0);
        if(cap < 30)  cap = 30;
        if(cap > 240) cap = 240;
        m_videoRenderer.setQueueFrames(cap);
        std::cout<<"视频队列上限 "<<cap<<" 帧(约2秒 @ "<<fps<<"fps)"<<std::endl;

        if(!m_videoRenderer.init(video_par,&m_audioPlayer))
        {
            std::cerr<<"视频渲染初始化失败"<<std::endl;
            return false;
        }
    }

    return true;
}


bool Player::demuxLoop()
{   
    int failCount = 0;      //连续读失败计数,直播流允许重试(必须在 while 外,否则每轮都归零)
    
    while(m_isRunning)
    {   
        if(m_isPaused || m_isSeeking)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        //新增关于对点播和直播的不同处理，点播等待，直播丢帧
        if(!m_isLive && m_videoRenderer.queueSize()>= m_videoRenderer.queueLimit())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        AVPacketPtr packet(av_packet_alloc());
        int ret = 0;
        {
        std::lock_guard<std::mutex> lock(m_seekMutex);
            
        ret = av_read_frame(m_avformat.get(),packet.get());
        if(ret==AVERROR_EOF)
        {
            std::cout<<"读取到文件结尾"<<std::endl;
            break;
        }
        else if(ret<0)
        {
            //直播流网络抖动是常态,一次失败就 break 会让画面永久冻死。
            //重试 50 次(约 1 秒)仍失败才真正退出
            if(m_isLive && ++failCount < 50)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }
            std::cout<<"读取文件失败"<<std::endl;
            break;
        }
        failCount = 0;      //读到数据就清零

        //第一个带pts的包,确认流的起点。
        //必须放在 av_read_frame 之后 —— 只有读成功了 packet 里才有数据
        if(!m_baseInited && packet->pts != AV_NOPTS_VALUE)
        {
            m_baseTime = packet->pts * av_q2d(m_avformat->streams[packet->stream_index]->time_base);
            m_audioPlayer.set_clock(0.0);
            m_audioPlayer.setTimeBase(m_audioTimeBase,m_baseTime);
            m_baseInited = true;
            m_videoRenderer.m_baseTime = m_baseTime;
        }
        
        static bool once = true;
        if(once && packet->stream_index == m_videoStreamIndex)
        {
            once = false;
            std::cout<<"video first pts="<<packet->pts
                     <<"tb="<<m_videoTimeBase.num<<"/"<<m_videoTimeBase.den
                     <<std::endl;
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
    }
    //睡眠一秒防止循环跑满
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
 }
    std::cout<<"解复用进程退出"<<std::endl;
    return true;
}

void Player::pause()
{  if(m_isPaused || !m_isRunning)
    return;
    m_isPaused = true;
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
    m_isPaused = false;
    m_audioPlayer.play();
    m_videoRenderer.play();
    //核心解复用进程
    m_demuxThread = std::thread(&Player::demuxLoop,this);
    std::cout<<"开始播放"<<std::endl;
}
void Player::stop()
{
    m_isRunning = false;

    if(m_demuxThread.joinable())
    {
        m_demuxThread.join();
    }
    m_audioPlayer.stop();
    m_videoRenderer.stop();

    std::cout << "播放器已停止" << std::endl;
}

void Player::resume()
{   if(!m_isRunning || !m_isPaused)
    return;
    //唤醒解复用线程
    m_isPaused = false;
    //这里音频的play只是把sdl取消暂停可以直接调用，但是视频需要调用resume来修改标志位，
    //把m_isPaused修改成false
    m_audioPlayer.play();
    m_videoRenderer.resume();
   
}

bool Player::getPauseState() const
{
    return m_isPaused;
}
void Player::renderVideoFrame()
{
    m_videoRenderer.renderOneFrame();
}

double Player::getCurrentTime() const
{   
    //使用音频时钟作为主时钟
    return m_audioPlayer.get_clock();
}
double Player::getDuration() const
{
    return m_duration;
}
SDL_Renderer* Player::getRenderer()
{
    return m_videoRenderer.getRenderer();
}
int Player::getWindowHeight() const
{
    return m_videoRenderer.getHeight();
}
int Player::getWindowWidth() const
{
    return m_videoRenderer.getWidth();
}


bool Player::seek(double time)
{   
    //限制直播流无法使用
    if(m_isLive)
    {
        std::cout<<"直播流不支持seek"<<std::endl;
        return false;
    }

    //1.限制拖动范围
    if( time<0.0)
    {
        time = 0.0;
    }
    else if(time>m_duration)
    {
        time = m_duration;
    }
    m_isSeeking = true;{
        std::lock_guard<std::mutex> lock(m_seekMutex);
        //转换pts
        int64_t timestmp = (int64_t)(time * AV_TIME_BASE);
        if(av_seek_frame(m_avformat.get(),-1,timestmp,AVSEEK_FLAG_BACKWARD)<0)
        {
            std::cerr<<"seek失败"<<std::endl;
            m_isSeeking = false;
            return false;
        }
        //清空队列和解码器
        m_audioPlayer.clearqueue();
        m_videoRenderer.clearqueue();
        if(m_audioDecoder) m_audioDecoder->flush();
        if(m_videoDecoder) m_videoDecoder->flush();
    }
    //重置playerbyte
    m_audioPlayer.set_clock(time);
    m_isSeeking = false;
    std::cout<<"跳转到"<<time<<"秒"<<std::endl;
    return true;
}

//转发同步到视频
bool Player::takeFrameDrawn()
{
    return m_videoRenderer.takeFrameDrawn();
}