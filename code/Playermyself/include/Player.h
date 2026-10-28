#pragma once

#include <iostream>
#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include<memory>
#include "FFmpeg.h"
#include "AudioPlayer.h"
#include "VideoPlayer.h"
#include "Decoder.h"
#include "ThreadSafeQueue.h"
class Player{
public:
Player();
~Player();
bool file_deal(const std::string& file);
void play();
void stop();
void pause();

//进度条
int getWindowWidth();
int getWindowHeight();
SDL_Renderer* getRenderer();
double getCurrentTime();
double getDuration();
bool seek(double time);
void renderVideoFrame();        //主线程里渲染每一帧
bool isPaused() const;          //空格判断暂停
void resume();                  //空格继续
private:
bool init_decoder();
void demuxloop();
bool findstream();

AVFormatContextPtr av_fmt_ctx;
std::atomic<bool> m_isRunning{false};
std::atomic<bool> m_isPaused{true};
AudioPlayer  m_audioplayer;
VideoPlayer  m_videoplayer;
std::unique_ptr<Decoder> audio_decoder;
std::unique_ptr<Decoder> video_decoder;

std::thread demuxThread;
ThreadSafeQueue<AVPacketPtr> audiopacketqueue;
ThreadSafeQueue<AVPacketPtr> videopacketqueue;

int audio_stream_index = -1;
int video_stream_index = -1;

AVRational audio_timebase;
AVRational video_timebase;

//进度条参数
double m_duration = 0.0;            //总时长
std::mutex m_seekMutex;             //防止av_read_frame和av_seek_frame冲突
std::atomic<bool> m_isSeeking{false};   //判断


};