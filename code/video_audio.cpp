#include<iostream>
#include<SDL2/SDL.h>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
}


int main(int argc, char const *argv[])
{
    
    //创建文件上下文
    AVFormatContext* fmt_av_ctx = nullptr;

    //打开文件
    avformat_open_input(&fmt_av_ctx,"/mnt/hgfs/ffmpeg_assets/game_30.mp4",nullptr,nullptr);


    //解析文件流
    avformat_find_stream_info(fmt_av_ctx,nullptr);



    //找到视频流和音频流
    int av_audio_index = -1;
    int av_video_index =-1;
    for(int i=0;i<fmt_av_ctx->nb_streams;i++)
    {
        if(fmt_av_ctx->streams[i]->codecpar->codec_type== AVMEDIA_TYPE_VIDEO)
        {
            av_video_index=i;
        }

        if(fmt_av_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            av_audio_index=i;
        }
        
    }


    //创建音视频解码参数
    AVCodecParameters* audio_codecpar = fmt_av_ctx->streams[av_audio_index]->codecpar;
    AVCodecParameters* video_codecpar = fmt_av_ctx->streams[av_video_index]->codecpar;


    //找到对应解码器
    const AVCodec* video_decoder = avcodec_find_decoder(video_codecpar->codec_id);
    const AVCodec* audio_decoder = avcodec_find_decoder(audio_codecpar->codec_id);


    //创建解码器上下文
    AVCodecContext* codec_audio_ctx = avcodec_alloc_context3(audio_decoder);
    AVCodecContext* codec_video_ctx = avcodec_alloc_context3(video_decoder);

    //往解码器上下文里传入解码参数
    avcodec_parameters_to_context(codec_audio_ctx,audio_codecpar);
    avcodec_parameters_to_context(codec_video_ctx,video_codecpar);


    //打开解码器(初始化解码器)
    avcodec_open2(codec_audio_ctx,audio_decoder,nullptr);
    avcodec_open2(codec_video_ctx,video_decoder,nullptr);


    //初始化SDL
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    

    //对于视频，创建视频格式转换器；对于音频，创建音频格式转换器，同时音频需要创建swr上下文；设置缓冲区
    AVFrame* audio_frame=av_frame_alloc();
    AVFrame* video_frame = av_frame_alloc();

    AVPacket* packet=av_packet_alloc();
    AVFrame* yuv420p=av_frame_alloc();
    //视频

        

    //初始SDL窗口
                int width  = codec_video_ctx->width;
                int height = codec_video_ctx->height;
        SDL_Window* window = SDL_CreateWindow(
                    "video_audio_player",
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
                SDL_Texture* texture = SDL_CreateTexture(
                    renderer,
                    SDL_PIXELFORMAT_IYUV,
                    SDL_TEXTUREACCESS_STREAMING,
                    width,
                    height
                );


        SwsContext* sws_ctx = 
            sws_getContext(
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
        yuv420p->width=width;
        yuv420p->height=height;
        yuv420p->format=AV_PIX_FMT_YUV420P;
        av_frame_get_buffer(yuv420p,32);

        //音频
        AVChannelLayout out_chlayout;
        av_channel_layout_default(&out_chlayout,2);

            SDL_AudioDeviceID device;
            SDL_AudioSpec spec{};
            spec.freq=44100;
            spec.format=AUDIO_S16SYS;
            spec.channels=2;
            spec.samples=1024;

            device = SDL_OpenAudioDevice(
                nullptr,
                0,
                &spec,
                nullptr,
                0
            );
            SDL_PauseAudioDevice(device,0);

            int output_linesize = 0;
        //分配音频output_buffer
        uint8_t* output_buffer=nullptr;
        av_samples_alloc(
            &output_buffer,
            &output_linesize,
            2,
            192000,
            AV_SAMPLE_FMT_S16,
            0
        );

        //创建swr上下文
        SwrContext* swr_ctx = nullptr;
        
        swr_alloc_set_opts2(
        &swr_ctx,
        &out_chlayout,
        AV_SAMPLE_FMT_S16,
        44100,
        &codec_audio_ctx->ch_layout,
        codec_audio_ctx->sample_fmt,
        codec_audio_ctx->sample_rate,
        0,
        nullptr
        );

        if (!swr_ctx || swr_init(swr_ctx) < 0) {
            fprintf(stderr, "swr_init failed\n");
            return -1;
        }
   //添加音频时间钟
    double audio_clock = 0;

    //初始化视频渲染时间，以及真实音频时间钟
    double last_renderer_time = 0;
    
    //主循环，读取音视频上下文内容，packet压缩数据，receive接受数据之后，送给视频和音频解码器解码成yuv和pcm再播放
        while(av_read_frame(fmt_av_ctx,packet)>=0)
        {

            //检测到音频流数据，解码播放
            if(packet->stream_index==av_audio_index)
            {   
                avcodec_send_packet(codec_audio_ctx,packet);

                while(avcodec_receive_frame(codec_audio_ctx,audio_frame)==0)
                {
                    int output_samples = swr_convert(
                    swr_ctx,
                    &output_buffer,
                    192000,
                    (const uint8_t**)audio_frame->data,
                    audio_frame->nb_samples
                    );

                    //播放器播放
                    int pcm_size = av_samples_get_buffer_size(
                        nullptr,
                        2,                  //声道数
                        output_samples,            //采样数量
                        AV_SAMPLE_FMT_S16,  //格式
                        1
                    );
        //防止音频列表堵塞
        while(SDL_GetQueuedAudioSize(device)>44100*2*2/5)
        {
            SDL_Delay(40);
        }           
        SDL_QueueAudio(device, output_buffer, pcm_size);


        audio_clock+=(double)output_samples / 44100;

        }
    }

            //检测到视频流数据，解码显示
            if(packet->stream_index==av_video_index)
            { 
                avcodec_send_packet(codec_video_ctx,packet);

                while(avcodec_receive_frame(codec_video_ctx,video_frame)==0)
                {   

                     //计算PTS
                    double video_pts = video_frame->pts * 
                    av_q2d(fmt_av_ctx->streams[av_video_index]->time_base);

                    sws_scale(
                    sws_ctx,
                    video_frame->data,
                    video_frame->linesize,
                    0,
                    height,
                    yuv420p->data,
                    yuv420p->linesize
                  );

                //时间同步比较

                double queued_time=
                SDL_GetQueuedAudioSize(device)
                / (44100.0 * 2 *2);
                //此处加入了优化音频队列等待消耗的时间
                double real_audio_clock=audio_clock - queued_time;

                double delay = video_pts - real_audio_clock;

                if(delay>0)
                {   
                    double wait_time = delay - last_renderer_time;

                    if(wait_time>0)
                    {
                        SDL_Delay(wait_time*1000);
                    }
                }

                //视频渲染用时优化，start
                Uint64 start = SDL_GetPerformanceCounter();


                //SDL显示
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


                SDL_RenderClear(renderer);

                SDL_RenderCopy(
                    renderer,
                    texture,
                    nullptr,
                    nullptr
                );
                
                SDL_RenderPresent(renderer);

            //视频渲染时间问题优化 end
            Uint64 end = SDL_GetPerformanceCounter();
            last_renderer_time = (double)(end-start) /SDL_GetPerformanceFrequency();
                

            }

            av_packet_unref(packet);

        }
    }
       
        //资源释放
        av_frame_free(&video_frame);
        av_frame_free(&audio_frame);
        av_frame_free(&yuv420p);
        av_packet_free(&packet);
        sws_freeContext(sws_ctx);
        av_freep(&output_buffer);
        swr_free(&swr_ctx);
        av_channel_layout_uninit(&out_chlayout);
        avcodec_free_context(&codec_audio_ctx);
        avcodec_free_context(&codec_video_ctx);
        avformat_close_input(&fmt_av_ctx);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_CloseAudioDevice(device);
        SDL_Quit();



    return 0;
}
