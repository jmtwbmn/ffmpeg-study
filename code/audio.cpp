#include<iostream>
#include<fstream>
#include<SDL2/SDL.h>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}
int main(int argc,const char* argv[])
{

    //1.初始化上下文
    AVFormatContext* fmt_audio_ctx = nullptr;

    //2.打开文件
    if(avformat_open_input(
        &fmt_audio_ctx,
        "/mnt/hgfs/ffmpeg_assets/game.mp4",
        nullptr,
        nullptr)<0)
        {
            std::cout<<"打开文件失败"
            <<std::endl;
            return -1;
        }
    

    //3.解析文件流
   if(avformat_find_stream_info(fmt_audio_ctx,nullptr)<0)
   {
        std::cout<<"解析流失败"<<std::endl;
        return -1;
   }

    //4.找到音频流
    int audio_stream_index = -1;
    for(int a=0;a<fmt_audio_ctx->nb_streams;a++)
    {
        if(fmt_audio_ctx->streams[a]->codecpar->codec_type
            ==AVMEDIA_TYPE_AUDIO)
        {
            audio_stream_index=a;
            break;
        }
    }

    //传入解码参数
    AVCodecParameters* audio_codecpar = fmt_audio_ctx->
        streams[audio_stream_index]->codecpar;


    //5.找到对应解码器
    const AVCodec* decoder = 
    avcodec_find_decoder(audio_codecpar->codec_id);
    

    //6.创建音频上下文
    AVCodecContext* audio_codec_ctx =
     avcodec_alloc_context3(decoder);

    //传入参数
    avcodec_parameters_to_context(audio_codec_ctx,audio_codecpar);

    //打开解码器
    if(avcodec_open2(audio_codec_ctx,decoder,nullptr)<0)
    {
            std::cout<<"打开解码器失败"<<std::endl;
            return -1;
    }

    //7.解码音频流
    bool running = true;
    SDL_Event event;
    
    //创建音频转换器
    AVFrame* frame=av_frame_alloc();
    AVPacket* packet=av_packet_alloc();

    SwrContext* swr_ctx = nullptr;

    AVChannelLayout out_chlayout;
    av_channel_layout_default(&out_chlayout,2);
    //创建swr上下文
    if(swr_alloc_set_opts2(
        &swr_ctx,
        &out_chlayout,
        AV_SAMPLE_FMT_S16,
        44100,
        &audio_codec_ctx->ch_layout,
        audio_codec_ctx->sample_fmt,
        audio_codec_ctx->sample_rate,
        0,
        nullptr
        )<0)
        {
            std::cout<<"创建音频转换器失败"<<std::endl;
            return -1;
        }

        //初始化ffmpeg重采样上下文
        if(swr_init(swr_ctx)<0)
        {
            std::cout<<"swr初始化失败"<<std::endl;
            return -1;
        }



    //创建pcm文件
    std::ofstream pcm_file(
        "/mnt/hgfs/ffmpeg_assets/output1.pcm",
        std::ios::binary
    );
    if(!pcm_file)
    {
        std::cout<<"创建PCM文件失败"<<std::endl;
        return -1;
    }

   

    //分配缓冲区
    int max_output_samples = 4096;

    uint8_t* output_buffer = nullptr;
    int output_linesize = 0;

    if(av_samples_alloc(
        &output_buffer,
        &output_linesize,
        2,
        max_output_samples,
        AV_SAMPLE_FMT_S16,
        0)<0)
        {
            std::cout<<"PCM缓冲区创建失败"<<std::endl;
            return -1;
        }
    //主循环读取并写入pcm
    while(av_read_frame(fmt_audio_ctx,packet)>=0)
    {
       
        while(SDL_PollEvent(&event))
        {
            if(event.type ==SDL_QUIT)
            running=false;
        }
        if(packet->stream_index==audio_stream_index)
        {avcodec_send_packet(audio_codec_ctx,packet);


           while(avcodec_receive_frame(audio_codec_ctx,frame)==0)
           {
                int output_samples = swr_convert(
                    swr_ctx,
                    &output_buffer,
                    max_output_samples,
                    (const uint8_t**)frame->data,
                    frame->nb_samples
                );
                int pcm_size = av_samples_get_buffer_size(
                    &output_linesize,
                    2,
                    output_samples,
                    AV_SAMPLE_FMT_S16,
                    1
                );
            pcm_file.write(
                (char*)output_buffer,
                pcm_size
            );
           }
        }
        av_packet_unref(packet);
    }



    //9.初始化SDL
    if(SDL_Init(SDL_INIT_AUDIO)!=0)
    {
        std::cout<<"初始化失败"<<SDL_GetError()<<std::endl;
        return -1;
    }

    //10.设置音频参数
    SDL_AudioSpec spec{};
    spec.freq=44100;
    spec.format=AUDIO_S16SYS;
    spec.channels=2;
    spec.samples=1024;

    //11.打开音频设备
    SDL_AudioDeviceID device = SDL_OpenAudioDevice(
        nullptr,
        0,
        &spec,
        nullptr,
        0
    );
    if(device==0)
    {
        std::cout
        <<"打开音频失败"
        <<SDL_GetError()
        <<std::endl;
        return -1;
    }

    //12.打开pcm文件
    std::ifstream file("/mnt/hgfs/ffmpeg_assets/output1.pcm",std::ios::binary);


    //13.管理音频设备
    SDL_PauseAudioDevice(device,0);

    char buffer[4096];

    //14.读取pcm文件
    while(file)
    {
        file.read(buffer,sizeof(buffer));
        std::streamsize size = file.gcount();
        if(size>0)
        {
            SDL_QueueAudio(device,buffer,static_cast<Uint32>(size));

        }

    }
    
    //15.等待数据读取完毕
    while(SDL_GetQueuedAudioSize(device)>0)
    {
        SDL_Delay(100);
    }

    //16.释放资源
    
    
    SDL_CloseAudioDevice(device);
    file.close();
    SDL_Quit();

    av_freep(&output_buffer);
    swr_free(&swr_ctx);
    av_channel_layout_uninit(&out_chlayout);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&audio_codec_ctx);
    avformat_close_input(&fmt_audio_ctx);

    return 0;


}
