#pragma once
#include<iostream>
#include<SDL2/SDL.h>
#include<FFmpeg.h>
#include "FFmpeg.h"
#include<ThreadSafeQueue.h>
#include<atomic>
#include<thread>
class VideoPlayer{

public:
VideoPlayer();
~VideoPlayer();

VideoPlayer(const VideoPlayer&) = delete;
VideoPlayer& operator=(const VideoPlayer&) = delete;


bool video_init(AVCodecParameters* video_codec_par);

void play();
void pause();
void stop();
void clear();
void renderoneframe();
void renderloop();
void pushvideoQueue(AVFramePtr video_frame);

private:
bool sdl_init();
bool sws_init(AVCodecParameters* video_codec_par);

ThreadSafeQueue<AVFramePtr> videoframeQueue;
AVFramePtr video_frame;
AVFramePtr yuv420p;
AVPixelFormat pix_format = AV_PIX_FMT_NONE;
SDL_WindowPtr m_window;
SDL_TexturePtr m_texture;
SDL_RendererPtr m_renderer;
SwsContextPtr sws_ctx;

std::atomic<bool> m_isRunning{false};
std::atomic<bool> m_isPaused{true};
int width = 0;
int height = 0;
std::thread m_renderThread;

};