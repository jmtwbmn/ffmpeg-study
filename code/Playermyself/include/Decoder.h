#pragma once
#include<iostream>
#include<SDL2/SDL.h>
#include "FFmpeg.h"

extern "C"
{
    #include<libavcodec/avcodec.h>
}
//这里主要是写解码器的创建，传参，启动，以及传包

class Decoder{
public:
//构造和析构函数
 Decoder();
 virtual ~Decoder();

//禁止拷贝构造以及避免深拷贝
 Decoder(const Decoder*) = delete;
 Decoder operator = (const Decoder*) = delete;

bool open(AVCodecParameters* codec_par);
bool sendPacket(AVPacket* packet);
bool receiveFrame(AVFrame* frame);

//此处用protected,因为后续的auido_decoder和video_decoder要继承这个decoder
protected:
AVCodecContextPtr codec_par;
bool m_isOpened = false;
bool flush();
};


