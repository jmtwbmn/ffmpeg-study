#include<iostream>
#include<SDL2/SDL.h>
#include<fstream>

 int main(int argc, char const *argv[])
 {
   
    //1.初始化SDL
    SDL_Init(SDL_INIT_AUDIO);

    //2.设置音频格式
    SDL_AudioSpec spec{};
        spec.freq=44100,
        spec.format=AUDIO_S16SYS,
        spec.channels=2,
        spec.samples=1024;
    

    //3.打开SDL音频设备
    SDL_AudioDeviceID device=SDL_OpenAudioDevice(
        nullptr,
        0,
        &spec,
        nullptr,
        0
    );


    //4.打开PCM文件
    std::ifstream file("/mnt/hgfs/ffmpeg_assets/output.pcm",std::ios::binary);
    if(!file)
    {
    std::cout<<"open pcm failed"<<std::endl;
    return -1;
    }
    else
    {
    std::cout<<"open pcm success"<<std::endl;
    }


    //5.管理音频设备
    SDL_PauseAudioDevice(device,0);


    char buffer[4096];
    //6.读取PCM文件
    while(file)
    {
        file.read(buffer,sizeof(buffer));
        std::streamsize size = file.gcount();
        std::cout << "read bytes = " << size << std::endl;
        if(size>0)
        {
            SDL_QueueAudio(device,buffer,static_cast<Uint32>(size));
            
        }

    }

    //7.等待音频播完
    while(SDL_GetQueuedAudioSize(device)>0)
    {
        SDL_Delay(100);
        
    }


    SDL_CloseAudioDevice(device);

    file.close();
    SDL_Quit();

    return 0;
 }