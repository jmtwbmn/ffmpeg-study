#include<iostream>
#include<SDL2/SDL.h>
#include "Decoder.h"


 Decoder::Decoder(){}
 Decoder::~Decoder(){};

 
 bool Decoder::open(AVCodecParameters* codec_par)
 {
    const AVCodec* decoder = avcodec_find_decoder(codec_par->codec_id);
    if(!decoder)
    {
        std::cerr<<"解码器未找到对应数据"<<std::endl;
        return false;
    }
    codec_ctx.reset(avcodec_alloc_context3(decoder));
    if(!codec_ctx)
    {
        std::cerr<<"传入上下文失败"<<std::endl;
    }
    if(avcodec_parameters_to_context(codec_ctx.get(),codec_par)<0)
    {
        std::cerr<<"传入解码参数失败"<<std::endl;
    }
    if(avcodec_open2(codec_ctx.get(),decoder,nullptr)<0)
    {
        std::cerr<<"打开解码器失败"<<std::endl;
    }


    return true;
 }
 
 bool Decoder::sendPacket(AVPacket* packet)
 {
    if(!codec_ctx || !m_isOpened)
    {
        return false;
    }


   int ret = avcodec_send_packet(codec_ctx.get(),packet);
    if(ret<0 && ret!=AVERROR(EAGAIN)&&ret!=AVERROR_EOF)
   {
        std::cerr<<"传压缩包失败"<<std::endl;
        return false;
    }
    return true;
 }

 bool Decoder::receiveFrame(AVFrame* frame)
 {
    if(!codec_ctx || !m_isOpened)
    {
        return false;
    }
    int ret = avcodec_receive_frame(codec_ctx.get(),frame);
    if(ret<0 && ret!=AVERROR(EAGAIN)&& ret!=AVERROR_EOF)
    {
        std::cerr<<"接收帧数据失败"<<std::endl;
        return false;
    }
    
    return true;
 }
 
 void Decoder::flush()
 {
   if(codec_ctx.get())
   {
    avcodec_flush_buffers(codec_ctx.get());
   }
 }