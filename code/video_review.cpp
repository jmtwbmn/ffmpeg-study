#include<iostream>
#include<SDL2/SDL.h>

extern "C"
{
#include<libavformat/avformat.h>
#include<libavcodec/avcodec.h>
#include<libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
}


 int main(int argc, char const *argv[])
 {
    
    //创建文件上下文
    AVFormatContext* fmt_av_ctx = nullptr;


    //打开文件
    avformat_open_input(&fmt_av_ctx,"/mnt/hgfs/ffmpeg_assets/game_60.mp4",nullptr,nullptr);



    //解析文件流
    avformat_find_stream_info(fmt_av_ctx,nullptr);


    //找到音视频流
    int audio_index=-1;
    int video_index=-1;
    for(int i=0;i<fmt_av_ctx->nb_streams;i++)
    {
        if(fmt_av_ctx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO)
        {
            audio_index = i;
        }
        if(fmt_av_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_index = i;
        }

    }


    //创建音视频解码参数
    AVCodecParameters* audio_codecpar = fmt_av_ctx->streams[audio_index]->codecpar;
    AVCodecParameters* video_codecpar = fmt_av_ctx->streams[video_index]->codecpar;

    //找到对应解码器
    const AVCodec* audio_decoder = avcodec_find_decoder(audio_codecpar->codec_id);
    const AVCodec* video_decoder = avcodec_find_decoder(video_codecpar->codec_id);


    //创建音视频解码器上下文
    AVCodecContext* codec_audio_ctx=avcodec_alloc_context3(audio_decoder);
    AVCodecContext* codec_video_ctx=avcodec_alloc_context3(video_decoder);


    //传入解码参数到解码器上下文
    avcodec_parameters_to_context(codec_audio_ctx,audio_codecpar);
    avcodec_parameters_to_context(codec_video_ctx,video_codecpar);


    //启动解码器
    avcodec_open2(codec_audio_ctx,audio_decoder,nullptr);
    avcodec_open2(codec_video_ctx,video_decoder,nullptr);

    //初始化SDL，时间钟，渲染延时
    if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO)<0)
    {
        std::cout<<"初始化SDL失败"
                <<SDL_GetError()
                <<std::endl;
    }

    double audio_clock = 0;
    double last_renderer_time = 0;

    /*对于视频，创建ffmpeg转换器，创建sws_ctx上下文；对于音频，创建音频转换器，swr_ctx上下文，设置缓冲区；
        同时设置压缩包配置*/

    AVFrame* video_frame = av_frame_alloc();
    AVFrame* audio_frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();
    AVFrame* yuv420p = av_frame_alloc();

    //视频
     //初始化窗口
            int width = codec_video_ctx->width;
            int height = codec_video_ctx->height;
            SDL_Window* window = SDL_CreateWindow(
            "VIDEO_REVIEW",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width,
            height,
            SDL_WINDOW_SHOWN
        );

        SDL_Renderer* renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED
        );
            ////此时申请frame_alloc的width和height还没解码，使用解码器里的宽高
        SDL_Texture* texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_IYUV,
            SDL_TEXTUREACCESS_STREAMING,
            width,
            height
        );

        //sws_getContext(源W, 源H, 源格式, 目标W, 目标H, 目标格式, 算法, 源过滤, 目标过滤,参数)  3/3/1/3
            
    SwsContext* sws_ctx = sws_getContext(
        width,
        height,
        codec_video_ctx->pix_fmt,
        width,
        height,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr
    );
    yuv420p->width = width;
    yuv420p->height = height;
    yuv420p->format = AV_PIX_FMT_YUV420P;
    av_frame_get_buffer(yuv420p,32);


    //音频

    AVChannelLayout out_chlayout;
    //设置你想要的输出布局（例如：强制转为立体声）
    av_channel_layout_default(&out_chlayout,2);

    SDL_AudioDeviceID device;
    SDL_AudioSpec spec{};
    spec.freq = 44100;
    spec.channels =2;
    spec.format = AUDIO_S16SYS;
    spec.samples = 1024;

    device = SDL_OpenAudioDevice(
        nullptr,
        0,
        &spec,
        nullptr,
        0
    );

    SDL_PauseAudioDevice(device,0);

    
    int output_linesize = 0;

    // 【音频缓冲区分配】
    // av_samples_alloc: 为解码并重采样后的音频数据分配内存空间。
    // 参数1: &output_buffer -> 指向分配的内存首地址的指针。
    // 参数2: &output_linesize -> 每一行音频数据的字节数（音频里称为“步长”）。
    // 参数3: 2 -> 目标声道数（立体声，2个声道）。
    // 参数4: 192000 -> 分配足够大的采样点数量，防止转换后数据溢出。
    // 参数5: AV_SAMPLE_FMT_S16 -> 音频样本的格式（16位带符号整型，SDL最常用的音频格式）。
    // 参数6: 0 -> 对齐方式（0表示默认对齐）。
    uint8_t* output_buffer = nullptr;
    av_samples_alloc(
        &output_buffer,
        &output_linesize,
        2,
        192000,
        AV_SAMPLE_FMT_S16,
        0
    );
    

    // 【音频重采样器上下文创建】
    SwrContext* swr_ctx = nullptr;

    // swr_alloc_set_opts2: 分配并设置音频重采样上下文（SwrContext）的参数。
    // 它的作用是把“输入音频（源）”的参数转换成“输出音频（目标）”的参数。
    swr_alloc_set_opts2(
        &swr_ctx,
        &out_chlayout,                  // [输出] 声道布局，我们设置了立体声（2声道）
        AV_SAMPLE_FMT_S16,              // [输出] 采样格式，设置为SDL需要的16位整型
        44100,                          // [输出] 采样率，设置为44100Hz
        &codec_audio_ctx->ch_layout,    // [输入] 源音频的声道布局（从解码器上下文获取）
        codec_audio_ctx->sample_fmt,    // [输入] 源音频的采样格式（可能是FLTP、S16等）
        codec_audio_ctx->sample_rate,   // [输入] 源音频的采样率（可能是48000、44100等）
        0,                              // 标志位，默认填0
        nullptr                         // 日志上下文，默认填nullptr
    );

    // swr_init: 必须调用这个函数，底层才会真正根据上面设置的参数去计算重采样所需的滤波器系数等数据。
    // 如果没有调用 swr_init，后续的 swr_convert 会失败或崩溃。
    if(!swr_ctx || swr_init(swr_ctx)<0)   
    {
        std::cout<<"faleid to swr_init"
                 <<SDL_GetError()
                 <<std::endl;
    }


    //主循环，读取上下文数据，send给packet压缩，receive接收之后送给解码器解码成pcm,yuv后播放
    bool running = true;
    SDL_Event event;

    while(av_read_frame(fmt_av_ctx,packet)>=0)
    {

        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        //处理音频

        if(packet->stream_index == audio_index)
        {
            avcodec_send_packet(codec_audio_ctx,packet);
            
            while(avcodec_receive_frame(codec_audio_ctx,audio_frame)==0)
            {

                // 【音频重采样核心函数】
                // swr_convert: 将音频数据从源格式转换为目标格式。
                // 参数1: swr_ctx -> 上面初始化好的重采样上下文。
                // 参数2: &output_buffer -> 输出缓冲区，转换后的音频数据会写到这里。
                // 参数3: 192000 -> 输出缓冲区能容纳的最大采样点数。
                // 参数4: (uint8_t **)audio_frame->data -> 输入音频数据指针（指向解码出来的AVFrame）。
                // 参数5: audio_frame->nb_samples -> 输入音频的采样点数量。
                // 返回值 output_samples: 实际转换出的采样点数量。
                int output_samples = swr_convert(
                    swr_ctx,
                    &output_buffer,
                   192000,
                   (uint8_t **)audio_frame->data,
                   audio_frame->nb_samples
                );

                // av_samples_get_buffer_size: 根据采样点数量、声道数、格式，计算出这段PCM音频数据占用的字节大小。
                // 因为SDL_QueueAudio需要明确的字节大小。
                int pcm_size = av_samples_get_buffer_size(
                    nullptr,
                    2,
                    output_samples,
                    AV_SAMPLE_FMT_S16,
                    1
                );

                // 防止音频列表堵塞: 如果SDL声卡缓冲区排队的数据超过了1/5秒(44100*2*2/5字节)，就等一会儿。
                // 如果不加这个限制，程序会疯狂读文件，导致内存暴涨且音视频不同步。
                while(SDL_GetQueuedAudioSize(device) > 44100*2*2/5)
                {
                    SDL_Delay(40);
                }

                // 将转换好的PCM数据丢给声卡播放。
                SDL_QueueAudio(
                    device,
                    output_buffer,
                    pcm_size
                );

                // 更新音频时钟: 当前已播放的音频时长(秒) = 累计的采样点数 / 采样率(44100)
                audio_clock += (double)output_samples / 44100 ;


            }

        }
        //视频处理
            if(packet->stream_index == video_index)
            {
                avcodec_send_packet(codec_video_ctx,packet);

                while(avcodec_receive_frame(codec_video_ctx,video_frame)==0)
                {
                   //计算pts
                   double video_pts = video_frame->pts * av_q2d(fmt_av_ctx->streams[video_index]->time_base);

                    // 【视频缩放/格式转换核心函数】
                    // sws_scale: 将解码后的视频帧缩放并转换为目标格式（这里是YUV420P）。
                    // 参数1: sws_ctx -> 初始化好的视频转换上下文。
                    // 参数2: video_frame->data -> 输入图像的数据指针（指向解码出的YUV各个平面）。
                    // 参数3: video_frame->linesize -> 输入图像的行大小（每一行占用的字节数，包含内存对齐的填充）。
                    // 参数4: 0 -> 从输入图像的第几行开始处理（0表示从最顶部开始）。
                    // 参数5: height -> 要处理多少行（这里是整帧高度）。
                    // 参数6: yuv420p->data -> 输出图像的数据指针（转换后的图像写入这里）。
                    // 参数7: yuv420p->linesize -> 输出图像的行大小。
                    sws_scale(
                        sws_ctx,
                        video_frame->data,
                        video_frame->linesize,
                        0,
                        height,
                        yuv420p->data,
                        yuv420p->linesize
                    );

                    // 计算SDL音频队列中缓存的时间(秒)。44100.0*2*2 = 采样率*声道数*每样本字节数(S16)。
                    double queuetime = SDL_GetQueuedAudioSize(device) / (44100.0 *2 *2 );
                    
                    // 真实的音频播放时钟 = 已经入队的音频总时长 - 还在缓冲区没播完的时长。
                    double real_audio_clock = audio_clock - queuetime;

                    // 计算视频应该等待的时间: 视频当前帧的PTS - 当前实际音频播放进度。
                    double delay = video_pts - real_audio_clock;

                    if(delay>0)
                    {
                        // 减去上一帧渲染消耗的时间，避免延迟累积（精确控制）。
                        double wait_time = delay - last_renderer_time;
                        if(wait_time>0)
                        {
                            // 如果视频比音频快，就延迟等待，等音频追上来（音视频同步策略：音频为主时钟）。
                            SDL_Delay(wait_time*1000);
                        }

                    }

                    // 视频渲染用时start
                    Uint64 start = SDL_GetPerformanceCounter();

                    // 将转换后的YUV数据更新到SDL的纹理中。
                    SDL_UpdateYUVTexture(
                        texture,
                        nullptr,
                        yuv420p->data[0],   // Y平面（亮度）
                        yuv420p->linesize[0],
                        yuv420p->data[1],   // U平面（色度）
                        yuv420p->linesize[1],
                        yuv420p->data[2],   // V平面（色度）
                        yuv420p->linesize[2]
                    );

                    SDL_RenderClear(renderer);  // 清空当前渲染目标。
                    SDL_RenderCopy(            // 将纹理拷贝到渲染器。
                        renderer,
                        texture,
                        nullptr,
                        nullptr
                    );
                    SDL_RenderPresent(renderer); // 将渲染的结果显示到屏幕上（双缓冲交换）。

                    // 视频渲染用时 end
                    Uint64 end = SDL_GetPerformanceCounter();

                    // 计算这一帧渲染耗费的时间（秒），用于下一次的同步计算。
                    last_renderer_time = (double)(end-start) / SDL_GetPerformanceFrequency();

                }
                

            }

            av_packet_unref(packet);
        }


            //释放资源

            av_frame_free(&audio_frame);
            av_frame_free(&video_frame);
            av_frame_free(&yuv420p);
            av_packet_free(&packet);
            
            // swr_free: 释放音频重采样上下文。
            // 注意传入的是 &swr_ctx （二级指针），释放后会将 swr_ctx 置为 nullptr。
            swr_free(&swr_ctx);

            // sws_freeContext: 释放视频缩放/格式转换上下文。
            // 注意：这个函数名是驼峰式，且传入的是指针本身，而不是它的地址。
            sws_freeContext(sws_ctx);

            // av_freep: 安全释放音频缓冲区，同样传入二级指针，并自动置空。
            av_freep(&output_buffer);

            // av_channel_layout_uninit: 释放声道布局占用的动态内存（如果有的话）。
            av_channel_layout_uninit(&out_chlayout);

            avformat_close_input(&fmt_av_ctx);
            avcodec_free_context(&codec_video_ctx);
            avcodec_free_context(&codec_audio_ctx);
            SDL_DestroyWindow(window);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyTexture(texture);
            SDL_Quit();
            SDL_CloseAudioDevice(device);
            








    return 0;
 }