#include<iostream>
#include<string>
#include<thread>
#include<chrono>
#include<SDL2/SDL.h>
#include<SDL2/SDL_events.h>
#include "Player.h"
int main(int argc, char const *argv[])
{   
    //检测命令行参数，传入视频路径
    if(argc<2)
    {
        std::cerr<<"用法:"<<argv[0]<<"视频路径"<<std::endl;
        return -1;
    }
    std::string url = argv[1];
    //实例化
    Player player;
    //打开文件并初始化模块
    if(!player.file_deal(url))
    {
        std::cerr<<"文件打开/初始化失败，检查路径"<<std::endl;
        return -1;
    }
    //开始播放
    player.play();
    std::cout<<"开始播放"<<std::endl;
    
    bool isRunning = true;
    while(isRunning)
    {
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            if(event.type==SDL_QUIT)
            {
                std::cout<<"用户手动退出"<<std::endl;
                isRunning = false;
            }
            //键盘事件：esc退出
            if(event.type == SDL_KEYDOWN)
            {   
                if(event.key,keysym.sym == SDLK_ESCAPE)
                {
                    std::cout<<"用户按下esc退出"<<std::endl;
                    isRunning = false;
                }
            }
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    //停止线程，释放资源
    player.stop();
    std::cout<<"程序正常退出"<<std::endl;
    return 0;
}
