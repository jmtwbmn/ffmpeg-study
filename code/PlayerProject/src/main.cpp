#include "Player.h"
#include<iostream>
#include<thread>
#include<chrono>
#include<SDL2/SDL.h>

int main(int argc, char const *argv[])
{
    Player player;
    if(!player.open("/mnt/hgfs/ffmpeg_assets/game_60.Mp4"))
    {
        std::cerr<<"打开失败"<<std::endl;
        
        return -1;
    }
    player.play();
   
    bool running = true;
    while(running)
    {
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
            {
                running = false;
            }

        }
        player.render();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    player.stop();
    return 0;
}
