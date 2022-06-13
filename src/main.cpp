#define SDL_MAIN_HANDLED

#include <iostream>
#include <vulkan/vulkan.h>
#include "SDL.h"

int main(int argc, char *argv[])
{
    std::cout << "Hello World!" << std::endl;

    //SDL open window
    const int screen_width = 1280;
    const int screen_height = 720;
    SDL_Window *window = nullptr;
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "SDL could not initialize! Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    window = SDL_CreateWindow("Vulkan Intro", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN);
    if(window == nullptr)
    {
        std::cout << "Could not create window: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_ShowWindow(window);
    bool running = true;
    SDL_Delay(5000);

    //Main engine loop
    while(running)
    {
        SDL_Event event;
        SDL_PollEvent(&event);
        switch(event.type)
        {
            case SDL_QUIT:
                running = false;
                break;
        }
    }

    // Quit application
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}