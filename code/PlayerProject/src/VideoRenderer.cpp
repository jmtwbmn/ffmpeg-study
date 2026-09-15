#include<iostream>
#include<SDL2/SDL.h>
#include "VideoRenderer.h"
#include "FFmpegPtr.h"
#include "ThreadSafeQueue.h"
bool VideoRenderer::init(AVCodecParameters* codec_par,AudioPlayer* audioPlayer)
{
   
   
   int width = codec_par->width;
   int height = codec_par->height;

    bool sdl_init(width,height)
    {
        if(SDL_WindowPtr window = SDL_CreateWindow(
            "audio.player",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width,
            height,
            SDL_WINDOWSHOWN
        )<0)
        {
            std::cerr<<"创建窗口失败"<<std::endl;
            return false;
        };
        
        if(SDL_RenderPtr renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED
        )<0)
        {
            std::cerr<<"创建渲染器失败"<<std::endl;
            return false;
        };

        if(SDL_TexturePtr texture = SDL_CreateTexture(
            renderer,
            width,
            height,
            SDL_PIXFORMAT_IYUV,
            SDL_TEXTUREACCESS_STREAMING
        )<0)
        {
            std::cerr<<"创建纹理失败"<<std::endl;
            return false;
        };

        return true;

    };


    bool sws_init(AVCodecParametersPtr codec_par)
    {
        AVFramePtr audio_frame = av_frame_alloc();
        AVPacketPtr audio_packet = av_packet_alloc();
        AVFramePtr yuv420p = av_frame_alloc();

        yuv420p->width=width;
        yuv420p->height=height;
        yuv420p->foramt=av_format



    }







}