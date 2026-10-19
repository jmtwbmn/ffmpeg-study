#include <SDL2/SDL.h>
#include <iostream>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

int main()
{
    // 第一步：打开文件
    AVFormatContext* fmt_ctx = nullptr;

    if (avformat_open_input(
            &fmt_ctx,
            "/mnt/hgfs/ffmpeg_assets/game.mp4",
            nullptr,
            nullptr) < 0)
    {
        std::cout << "无法打开视频文件" << std::endl;
        return 1;
    }

    // 第二步：获取流信息
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0)
    {
        std::cout << "无法获取流信息" << std::endl;
        return 1;
    }

    // 第三步：找到视频流
    int video_index = -1;

    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++)
    {
        if (fmt_ctx->streams[i]->codecpar->codec_type
            == AVMEDIA_TYPE_VIDEO)
        {
            video_index = i;
            break;
        }
    }

    if (video_index == -1)
    {
        std::cout << "没有找到视频流" << std::endl;
        return 1;
    }

    // 第四步：获取编码参数
    AVCodecParameters* codecpar =
        fmt_ctx->streams[video_index]->codecpar;

    // 第五步：找到解码器
    const AVCodec* decoder =
        avcodec_find_decoder(codecpar->codec_id);

    if (decoder == nullptr)
    {
        std::cout << "找不到解码器" << std::endl;
        return 1;
    }

    // 第六步：创建解码器上下文
    AVCodecContext* codec_ctx =
        avcodec_alloc_context3(decoder);

    // 把编码参数复制到上下文
    avcodec_parameters_to_context(
        codec_ctx,
        codecpar
    );

    // 打开解码器
    avcodec_open2(
        codec_ctx,
        decoder,
        nullptr
    );

    // 获取视频宽高
    int width = codec_ctx->width;
    int height = codec_ctx->height;


    // 第七步：初始化 SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "SDL初始化失败："
                  << SDL_GetError()
                  << std::endl;
        return 1;
    }

    // 第八步：创建窗口
    SDL_Window* window =
        SDL_CreateWindow(
            "FFmpeg SDL Player",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width,
            height,
            SDL_WINDOW_SHOWN
        );

    // 第九步：创建渲染器
    SDL_Renderer* renderer =
        SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED
        );

    // 第十步：创建纹理
    SDL_Texture* texture =
        SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_IYUV,
            SDL_TEXTUREACCESS_STREAMING,
            width,
            height
        );

            // 第十一步：创建 FFmpeg 转换器
    SwsContext* sws_ctx =
        sws_getContext(
            width,
            height,
            codec_ctx->pix_fmt,
            width,
            height,
            AV_PIX_FMT_YUV420P,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr
        );

    // 第十二步：创建 Packet 和 Frame
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* yuv420p = av_frame_alloc();

    // 给 YUV420P Frame 分配内存
    yuv420p->format = AV_PIX_FMT_YUV420P;
    yuv420p->width = width;
    yuv420p->height = height;

    av_frame_get_buffer(yuv420p, 32);


    // 第十三步：读取、解码、显示
    bool running = true;
    SDL_Event event;

    while (running &&
           av_read_frame(fmt_ctx, packet) >= 0)
    {
        // 处理 SDL 事件
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        // 只处理视频 Packet
        if (packet->stream_index == video_index)
        {
            // 压缩数据送入解码器
            avcodec_send_packet(
                codec_ctx,
                packet
            );

            // 取出解码后的 Frame
            while (avcodec_receive_frame(
                       codec_ctx,
                       frame) == 0)
            {
                // 第十四步：转换成 YUV420P
                sws_scale(
                    sws_ctx,
                    frame->data,
                    frame->linesize,
                    0,
                    height,
                    yuv420p->data,
                    yuv420p->linesize
                );

                // 第十五步：YUV 放入纹理
                SDL_UpdateYUVTexture(
                    texture,
                    nullptr,
                    yuv420p->data[0],
                    yuv420p->linesize[0],
                    yuv420p->data[1],
                    yuv420p->linesize[1],
                    yuv420p->data[2],
                    yuv420p->linesize[2]
                );

                // 第十六步：渲染
                SDL_RenderClear(renderer);

                SDL_RenderCopy(
                    renderer,
                    texture,
                    nullptr,
                    nullptr
                );

                SDL_RenderPresent(renderer);

                // 暂时控制播放速度
                SDL_Delay(40);
            }
        }

        // 释放当前 Packet
        av_packet_unref(packet);
    }


    // 第十七步：释放资源
    av_frame_free(&frame);
    av_frame_free(&yuv420p);

    av_packet_free(&packet);

    sws_freeContext(sws_ctx);

    avcodec_free_context(&codec_ctx);

    avformat_close_input(&fmt_ctx);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}