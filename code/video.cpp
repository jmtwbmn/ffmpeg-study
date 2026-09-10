#include<SDL2/SDL.h>
#include<iostream>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}


    //创建并初始化文件上下文 format_ctx
    AVformatContext* fmt_ctx=nullptr;
    
    //第一步，读取文件
    avformat_open_input(
        &fmt_ctx,
        "/mnt/hgfs/ffmpeg_assets/game.mp4",
        nullptr,
        nullptr
    );


    //第二步，解析文件流
    avformat_find_stream_info(
        fmt_ctx,
        nullptr
    );


    //第三步，找到视频流
    
    int video_index=-1;

    for(int i=0;i<fmt_ctx->nb_streams;i++)
    {

        if(
            fmt_ctx->stream[i]
            ->codecpar
            ->codec_type == AVMEDIA_TYPE_VIDEO
        )
        {
            video_index=i;
            break;
        }

    }


    //第四步，找到对应的解码器

    const AVcodec* decoder=fmt_ctx->streams[video_index]->coderpar;
    decoder=avcodec_find_decoder(codecpar->codec_id);



    //第五步，创建上下文
    AVcodecContext* codec_ctx=avcodec_alloc.context3(decoder);

    avcodec_parameters_to_context(
        codec_ctx,
        codepar
    );
    avcocdec_open2(
        codec_ctx,
        decoder,
        nullptr

    );


    //第六步，初始化SDL

    SDL_Init(SDL_INIT_VIDEO);



    //第七步，创建窗口
    SDL_Window* window=SDL_CreateWindow(
        "player",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN
    );


    //第八步，创建渲染器
    SDL_Renderer* renderer=SDL_CreateRenerder(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    

    //第九步，创建纹理
    SDL_Texture* texture=SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );
    



    //第十步，创建ffmpeg转换器
        //创建avpacket和avframe
        AVPacket* packet=av_packet_alloc();

        ACFrame* frame=av_frame_alloc();

    SwsContext* sws_ctx=sws_getContext(

        width,
        height,
        codec_ctx->pix_fmt,

        width,
        height,
        AV_PIX_FMT_YUV420P,

        SWS_BRLINEAR,
        nullptr,
        nullptr,
        nullptr
    );



    //十一步，开始循环解码，packet导入上下文，send给解码器，然后reveive，再用转换器转换格式

   
        while(av_frame(fmt_ctx,packet)>=0)
        {
            if(packet->stream_index==video_index)
            {

                avcodec_send_packet(
                    codec_ctx,
                    packet
                );

                while(
                    avcodec_receive_frame(
                    codec_ctx,
                    frame
                )==0
                {

                    //得到一帧
                }


            }
            av_packet_unref(packet);
        }

        //十二步，转换YUV
        AVFrame* yuv420p=av_frame_alloc();

        yuv420p->format=AV_PIX_FMT_YUV420p;

        yuv420p->width=width;
        yuv420p->height=height;

        av_frame_get_buffer(
            yuv420p,
            32
        );
        //转化
        sws_scale(
            sws_ctx,

            frame->data,
            frame->linesize,

            0,
            height,

            yuv420p->data,
            yuv420p->linesize
        );


        //十三步,视频传入纹理
        SDL_UpdateYUVTexture(
            texture,
            nullptr,

            yuv420p->data[0],
            yuv420p->linesize[0],

            yuv420p->data[1],
            yuv420p->linesize[1],

            yuv420p->data[2],
            yuv420p->linesize[2],

        )




        //十二步，刷新
        renderclear(renderer);

        //十三步，复制填充
        rendercopy(
            renderer,
            texture,
            nullptr,
            nullptr
        );

        //十四步，显示
        renderpresent(renderer);




    //最后释放资源
    av_frame_free(&frame);
    av_frame_free(&yuv420p);

    av_packet_free(&packet);

    sws_freeContext(sws_ctx);

    avcodec_free_Contex(&codec_ctx);

    avformat_clos_input(&fmt_ctx);

    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);

    SDL_Quit();
