#define SDL_MAIN_HANDLED

#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>
#include "SDL.h"
#include "SDL_vulkan.h"

int main(int argc, char *argv[])
{
    std::cout << "Hello World!" << std::endl;

    //SDL open window
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "SDL could not initialize! Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    const int screen_width = 1280;
    const int screen_height = 720;
    SDL_Window *window = SDL_CreateWindow("Vulkan Intro", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    if(window == nullptr)
    {
        std::cout << "Could not create window: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_ShowWindow(window);

    //Create Vulkan instance and query for extensions
    unsigned int extension_count = 0;
    if(!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr))
    {
        std::cout << "Could not get Vulkan instance extensions: " <<SDL_GetError() << std::endl;
        return 1;
    }
    std::vector<const char*> extensions = {VK_EXT_DEBUG_REPORT_EXTENSION_NAME};
    size_t additional_extensions_count = extensions.size();
    extensions.resize(additional_extensions_count + extension_count);
    if(!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data() + additional_extensions_count))
    {
        std::cout << "Could not get Vulkan instance extensions: " <<SDL_GetError() << std::endl;
        return 1;
    }

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = {};
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    create_info.pApplicationInfo = &app_info;
    create_info.enabledLayerCount = 0;

    VkInstance instance = {};
    if(vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
    {
        std::cout << "Failed to create Vulkan instance." << std::endl;
        return 1;
    }


    bool running = true;
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