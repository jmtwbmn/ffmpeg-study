#include <iostream>
#include <fstream>
extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

int main(int argc,char** argv)
{	
    av_register_all();
    AVFormatContext* fmt_ctx = nullptr;
    int ret = avformat_open_input(&fmt_ctx,argv[1],nullptr,nullptr);
    if(ret != 0)
    {
        std::cout<<"打开文件失败"<<std::endl;
        return -1;
    }
    avformat_find_stream_info(fmt_ctx,nullptr);

    std::cout << "文件名: " << argv[1] << std::endl;
    std::cout << "时长: " << fmt_ctx->duration / AV_TIME_BASE << " 秒" << std::endl;

    int video_stream_idx = -1;
    int audio_stream_idx = -1;
    AVCodecContext* video_codec_ctx = nullptr;
    AVCodecContext* audio_codec_ctx = nullptr;

    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream* stream = fmt_ctx->streams[i];
        AVCodecContext* codec_ctx = stream->codec;

        std::cout << "流 #" << i << ": ";
        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            video_codec_ctx = codec_ctx;
            AVCodec* dec = avcodec_find_decoder(codec_ctx->codec_id);
            avcodec_open2(codec_ctx,dec,nullptr);

            std::cout << "视频, 编码: " << avcodec_get_name(codec_ctx->codec_id)<< std::endl;
            std::cout << "分辨率:"<<codec_ctx->width <<"x"<<codec_ctx->height<<std::endl;
        }else if(codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_stream_idx = i;
            audio_codec_ctx = codec_ctx;
            AVCodec* dec = avcodec_find_decoder(codec_ctx->codec_id);
            avcodec_open2(codec_ctx,dec,nullptr);

            std::cout << "音频, 编码: " << avcodec_get_name(codec_ctx->codec_id)<< std::endl;
            std::cout << "采样率:"<<codec_ctx->sample_rate<<std::endl;
        }else{
            std::cout<<"其他流"<<std::endl;
        }
    }

    std::ofstream yuv_out("output.yuv", std::ios::binary);
    std::ofstream pcm_out("output.pcm", std::ios::binary);

    AVPacket pkt;
    av_init_packet(&pkt);
    AVFrame* frame = av_frame_alloc();

    while( av_read_frame(fmt_ctx, &pkt) == 0 )
    {
        if(pkt.stream_index == video_stream_idx)
        {
            int got_frame = 0;
            int r = avcodec_decode_video2(video_codec_ctx, frame, &got_frame, &pkt);
            if(got_frame)
            {
                int w = video_codec_ctx->width;
                int h = video_codec_ctx->height;
                yuv_out.write((char*)frame->data[0], frame->linesize[0]*h);
                yuv_out.write((char*)frame->data[1], frame->linesize[1]*h/2);
                yuv_out.write((char*)frame->data[2], frame->linesize[2]*h/2);
            }
        }
        else if(pkt.stream_index == audio_stream_idx)
        {
            int got_frame =0;
            int r = avcodec_decode_audio4(audio_codec_ctx, frame, &got_frame, &pkt);
            if(got_frame)
            {
                int samples = frame->nb_samples;
                int bytes_per_sample = av_get_bytes_per_sample(audio_codec_ctx->sample_fmt);
                int ch = audio_codec_ctx->channels;
                pcm_out.write((char*)frame->data[0],samples * bytes_per_sample * ch);
            }
        }
        av_free_packet(&pkt);
    }

    yuv_out.close();
    pcm_out.close();
    av_frame_free(&frame);

    avformat_close_input(&fmt_ctx);
    return 0;
}
