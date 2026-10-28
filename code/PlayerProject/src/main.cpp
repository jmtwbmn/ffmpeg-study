#include "Player.h"
#include<iostream>
#include<thread>
#include<chrono>
#include<SDL2/SDL.h>
#include "ProgressBar.h"

int main(int argc, char const *argv[])
{   
    if(argc <2)
    {
        std::cerr<<"用法"<<argv[0]<<"文件或者url"<<std::endl;
        return -1;
    }
    Player player;
    if(!player.open(argv[1]))
    {
        std::cerr<<"打开失败"<<std::endl;
        return -1;
    }
    player.play();
   
    //初始化进度条
    ProgressBar progressBar;
    progressBar.init(player.getWindowWidth(),player.getWindowHeight());
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

            // ===== 进度条处理鼠标事件 =====
            if(event.type == SDL_MOUSEMOTION)
            {   //拖动中只更新位置不seek，避免每一帧都seek
               progressBar.handleMouseMove(event.motion.x, event.motion.y);
            }
            
            if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {   
               //只记录状态，不seek
                progressBar.handleMouseDown(event.button.x, event.button.y);
            }
            
            if(event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
            {
                if(progressBar.handleMouseUp(event.button.x, event.button.y))
                //松手的时候seek
                {player.seek(progressBar.getSeekTime());}
            }

            //新增键盘事件，暂停和继续
            if(event.type == SDL_KEYDOWN)
            {
                switch(event.key.keysym.sym)
                {
                case SDLK_SPACE:
                    if(player.getPauseState())
                    {
                        player.resume();
                        std::cout<<"继续播放"<<std::endl;
                    }
                    else{
                        player.pause();
                        std::cout<<"暂停播放"<<std::endl;
                    }
                    break;
                case SDLK_LEFT:
                    player.seek(player.getCurrentTime() - 5.0);
                    break;
                case SDLK_RIGHT:
                    player.seek(player.getCurrentTime() + 5.0);
                    break;
                case SDLK_ESCAPE:
                    running = false;
                    break;
                }
            }

        }
        player.renderVideoFrame();
        //进度条更新绘制提交
        progressBar.update(player.getCurrentTime(),player.getDuration());
        if(player.takeFrameDrawn())
        {
            progressBar.draw(player.getRenderer());
            SDL_RenderPresent(player.getRenderer());
        }
             std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    player.stop();
    return 0;
}
