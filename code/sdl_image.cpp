#include<iostream>
#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>


int main(int argc, char** argv)
{
	//argc是传参个数，第二个指向指针字符数组
	if(argc<2)
	{
		std::cout<<" usage: ./sdl_image <bmp file>"<<std::endl;
		return -1;
	}

	//1.初始化SDL
	if(SDL_Init(SDL_INIT_VIDEO)!=0)
	{
		std::cout<<"SDL failed begin"
				 <<SDL_GetError()
				 <<std::endl;
		return -1;
	}
	//初始化image配置
	if(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) ==0)
	{
		std::cout<<"配置image失败"
				<<IMG_GetError()
				<<std::endl;
		return -1;
	}
	//2.创建窗口
	SDL_Window* window = SDL_CreateWindow(
		"Yasina Player",
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
	SDL_Surface* surface =IMG_Load(argv[1]);
	//argv[1]表示第二个参数，也就是文件的路径
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
		IMG_Quit();
		SDL_Quit();
	

	return 0;
}

