#include<iostream>
#include<SDL2/SDL.h>

int main(int argc, char** argv)
{
	//argc
	//1.初始化SDL
	if(SDL_Init(SDL_INIT_VIDEO)!=0)
	{
		std::cout<<"SDL failed begin"
				 <<SDL_GetError()
				 <<std::endl;
		return -1;
	}


	//2.创建窗口
	SDL_Window* window = SDL_CreateWindow(
		"SDL_Image Player",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		800,
		600,
		SDL_WINDOW_SHOWN
	);

	if(window == nullptr)
	{
		std::cout<<"windows failed to create"
				<<SDL_GetError()
				<<std::endl;

		SDL_Quit();
		return -1;
	}

	//3.创建Renderer
	SDL_Renderer* renderer = SDL_CreateRenderer(
		window,
		-1,
		SDL_RENDERER_ACCELERATED
	);

	if(renderer == nullptr)
	{
		std::cout<<"Renderer failed to create "
				<<SDL_GetError()
				<<std::endl;

		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}


	//4.加载bmp图片
	SDL_Surface* surface =SDL_LoadBMP("test.bmp");
		if(surface == nullptr)
		{
			std::cout<<"bmp failed to download"
					<<SDL_GetError()
					<<std::endl;

			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(window);
			SDL_Quit();
			return -1;

		}

	//5.Surface转Texture
		SDL_Texture* texture=
			SDL_CreateTextureFromSurface(renderer,surface);

		if(texture == nullptr)
		{
			std::cout<<"Texture failed to create "
					<<SDL_GetError()
					<<std::endl;

			SDL_FreeSurface(surface);
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(window);
			SDL_Quit();

			return -1;
		}
		//Surface已经没用
		SDL_FreeSurface(surface);



	//6.	主循环
		bool running =true;
		SDL_Event event;
		
		while(running)
		{
			//
			while(SDL_PollEvent(&event))
			{
				if(event.type == SDL_QUIT)
				{
					running = false;
				}

			}
			//清空窗口
			SDL_SetRenderDrawColor(
				renderer,
				0,
				0,
				0,
				255
			);

			SDL_RenderClear(renderer);

			//7.把Texture复制到Renderer
			SDL_RenderCopy(
				renderer,
				texture,
				NULL,
				NULL
			);

			//8.显示
			SDL_RenderPresent(renderer);

		}

		//9.释放资源
		SDL_DestroyTexture(texture);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
	

	return 0;
}

