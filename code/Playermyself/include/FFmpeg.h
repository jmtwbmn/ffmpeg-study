#pragma once
#include<iostream>
#include<memory>
#include<SDL2/SDL.h>


extern "C"{
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
};


//FFmpeg头文件写的内容，RALL封装各类指针，SDL和文件操作的

struct AVFrameDeleter{
    void operator()( AVFrame* frame)
    const{
        if(frame)
        {
            av_frame_free(&frame);
        }
    }
};
using AVFramePtr = std::unique_ptr<AVFrame,AVFrameDeleter>;

struct AVPacketDeleter{
    void operator()( AVPacket* packet)
    const{
        if(packet)
        {
            av_packet_free(&packet);
        }
    }
};
using AVPacketPtr = std::unique_ptr<AVPacket,AVPacketDeleter>;

struct AVCodecParametersDeleter{
    void operator()( AVCodecParameters* codec_par)
    const{
        if(codec_par)
        {
            avcodec_parameters_free(&codec_par);
        }
    }
};
using AVCodecParametersPtr = std::unique_ptr<AVCodecParameters,AVCodecParametersDeleter>;

struct AVFormatContextDeleter{
    void operator()( AVFormatContext* av_fmt_ctx)
    const{
        if(av_fmt_ctx)
        {
            avformat_close_input(&av_fmt_ctx);
        }
    }
};
using AVFormatContextPtr = std::unique_ptr<AVFormatContext,AVFormatContextDeleter>;

struct AVCodecContextDeleter{
    void operator()( AVCodecContext* codec_ctx)
    const{
        if(codec_ctx)
        {
            avcodec_free_context(&codec_ctx);
        }
    }
};
using AVCodecContextPtr = std::unique_ptr<AVCodecContext,AVCodecContextDeleter>;



//sdl部分  注，SDL销毁指针对象不接收指针地址
struct SDL_WindowDeleter{
    void operator()( SDL_Window* window)
    const{
        if(window)
        {
            SDL_DestroyWindow(window);
        }
    }
};
using SDL_WindowPtr = std::unique_ptr<SDL_Window,SDL_WindowDeleter>;

struct SDL_RendererDeleter{
    void operator()( SDL_Renderer* renderer)
    const{
        if(renderer)
        {
            SDL_DestroyRenderer(renderer);
        }
    }
};
using SDL_RendererPtr = std::unique_ptr<SDL_Renderer,SDL_RendererDeleter>;

struct SDL_TextureDeleter{
    void operator()(SDL_Texture* texture)
    const{
        if(texture)
        {
            SDL_DestroyTexture(texture);
        }
    }
};
using SDL_TexturePtr = std::unique_ptr<SDL_Texture,SDL_TextureDeleter>;

struct SwrContextDeleter{
    void operator()( SwrContext* swr_ctx)
    const{
        if(swr_ctx)
        {
            swr_free(&swr_ctx);
        }
    }
};
using SwrContextPtr = std::unique_ptr<SwrContext,SwrContextDeleter>;

struct SwsContextDeleter{
    void operator()( SwsContext* sws_ctx)
    const{
        if(sws_ctx)
        {
            sws_freeContext(sws_ctx);
        }
    }
};
using SwsContextPtr = std::unique_ptr<SwsContext,SwsContextDeleter>;
