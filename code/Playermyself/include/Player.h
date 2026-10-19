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
};