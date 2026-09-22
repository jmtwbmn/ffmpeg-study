
#pragma  once                       //避免多次重复定义

#include<iostream>
#include "FFmpegPtr.h"
extern "C"
{
    #include<libavcodec/avcodec.h>
}

class Decoder {

    public:
        Decoder();
        virtual ~Decoder();
        
        //禁用拷贝
        Decoder(const Decoder&) = delete;
        Decoder& operator=(const Decoder&) = delete;

        bool open(AVCodecParameters* codec_par);
        bool sendPacket(AVPacket* packet);
        bool receiveFrame(AVFrame* frame);
        void flush();

    protected:                  //此处是protected,因为之后audio,video Decoder要继承
        AVCodecContextPtr codec_ctx;    //智能指针
        bool m_isOpened = false;

};
