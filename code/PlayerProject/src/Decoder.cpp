#include<iostream>
#include "Decoder.h"



 bool Decoder::open(AVCodecParameters* codec_par)
 {
    if(!codec_par)
    {
        std::cerr<<"传入的AVCodecParameter为空"<<std::endl;
        return false;
    };

    //查找解码器
    const AVCodec* decoder = avcodec_find_decoder(codec_par->codec_id);
    if(!decoder)
    {
        std::cerr<<"无法找到对应解码器"<<std::endl;
        return false;
    };

    //分配解码器上下文,让智能指针管理
    codec_ctx.reset(avcodec_alloc_context3(decoder));
    if(!codec_ctx)
    {
        std::cerr<<"分配解码器上下文失败"<<std::endl;
        return false;
    };

    //传入解码参数
    if(avcodec_parameters_to_context(codec_ctx.get(),codec_par)<0)
    {
        std::cerr<<"传入解码参数失败"<<std::endl;
        return false;
    }

    //打开解码器
    if(avcodec_open2(codec_ctx.get(),decoder,nullptr)<0)
    {
        std::cerr<<"打开解码器失败"<<std::endl;
        return false;
    }

    m_isOpened = true;
    std::cout<<"初始化解码器成功"<<std::endl;

    return true;
 }

bool Decoder::sendPacket(AVPacket* packet) {
    if (!m_isOpened || !codec_ctx) return false;
    
    int ret = avcodec_send_packet(codec_ctx.get(), packet);
    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
        std::cerr << "[Decoder] 发送数据包失败！" << std::endl;
        return false;
    }
    return true;
}


 bool Decoder::receiveFrame(AVFrame* frame){
    if(!codec_ctx||!m_isOpened)
    {
        std::cerr<<"获取视频帧失败"<<std::endl;
        return false;
    }

    int ret = avcodec_receive_frame(codec_ctx.get(),frame);
      
        //缓冲区满了或者未满一帧
        if(ret ==  AVERROR(EAGAIN)||ret == AVERROR_EOF)
        {
            return false;
        }
        else if(ret <0)
        {
            return false;
        }
    
        return true;
    }

    void Decoder::flush()
        {
            if(codec_ctx.get())
            {
                avcodec_flush_buffers(codec_ctx.get());
                std::cout<<"缓冲区已刷新"<<std::endl;
            }
        }