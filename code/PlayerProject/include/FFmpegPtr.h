

//RALL封装内存管理



#include <memory>
#include <SDL2/SDL.h>

extern "C"
{
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
}

// ================== FFmpeg 封装 ==================

// AVFrame
struct AVFrameDeleter {
    void operator()(AVFrame* frame) const {
        if (frame) av_frame_free(&frame);
    }
};
using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;

// AVCodecContext
struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ctx) const {
        if (ctx) avcodec_free_context(&ctx);
    }
};
using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>; 

// SwrContext (音频重采样)
struct SwrContextDeleter {
    void operator()(SwrContext* swr_ctx) const {
        if (swr_ctx) swr_free(&swr_ctx);
    }
};
using SwrContextPtr = std::unique_ptr<SwrContext, SwrContextDeleter>; 

// SwsContext (视频缩放)
struct SwsContextDeleter {
    void operator()(SwsContext* sws_ctx) const {
        if (sws_ctx) sws_freeContext(sws_ctx);
    }
};
using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>; 

// AVFormatContext
struct AVFormatContextDeleter {
    void operator()(AVFormatContext* av_ctx) const { 
        if (av_ctx) avformat_close_input(&av_ctx);
    }
};
using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>; 

// ================== SDL 封装 ==================

// SDL_Window
struct SDL_WindowDeleter {
    void operator()(SDL_Window* window) const { // 补上 const
        if (window) SDL_DestroyWindow(window);
    }
};
using SDL_WindowPtr = std::unique_ptr<SDL_Window, SDL_WindowDeleter>; 

// SDL_Renderer
struct SDL_RendererDeleter {
    void operator()(SDL_Renderer* renderer) const { // 补上 const
        if (renderer) SDL_DestroyRenderer(renderer);
    }
};
using SDL_RendererPtr = std::unique_ptr<SDL_Renderer, SDL_RendererDeleter>; 

// SDL_Texture
struct SDL_TextureDeleter {
    void operator()(SDL_Texture* texture) const { // 修正函数名和参数
        if (texture) {
            SDL_DestroyTexture(texture); // 修正拼写错误
        }
    }
};
using SDL_TexturePtr = std::unique_ptr<SDL_Texture, SDL_TextureDeleter>; 