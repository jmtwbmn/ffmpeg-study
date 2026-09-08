#include <SDL2/SDL.h>
#include <iostream>

int main(int argc, char* argv[])
{
    // 初始化 SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "SDL 初始化失败: "
                  << SDL_GetError() << std::endl;
        return -1;
    }

    // 创建窗口
    SDL_Window* window = SDL_CreateWindow(
        "SDL2 Test",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800,
        600,
        SDL_WINDOW_SHOWN
    );

    if (window == nullptr)
    {
        std::cout << "创建窗口失败: "
                  << SDL_GetError() << std::endl;

        SDL_Quit();
        return -1;
    }

    // 创建渲染器
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    if (renderer == nullptr)
    {
        std::cout << "创建渲染器失败: "
                  << SDL_GetError() << std::endl;

        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // 主循环
    bool running = true;

    while (running)
    {
        SDL_Event event;

        // 处理事件
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }

            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    running = false;
                }
            }
        }

        // 设置背景颜色：白色
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        // 清空画布
        SDL_RenderClear(renderer);

        // 设置绘制颜色：黑色
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        // 黑色方框
        SDL_Rect rect;
        rect.x = 300;
        rect.y = 200;
        rect.w = 200;
        rect.h = 200;

        SDL_RenderFillRect(renderer, &rect);

        // 显示这一帧
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    // 释放资源
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
