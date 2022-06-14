#define SDL_MAIN_HANDLED

#include <iostream>
#include <vector>
#include <cstring>
#include <vulkan/vulkan.h>
#include "SDL.h"
#include "SDL_vulkan.h"

bool checkValidationLayerSupport(const std::vector<const char*> validation_layers)
{
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for(const char* layer_name : validation_layers)
    {
        bool layer_found = false;
        for(const auto& layer_properties : available_layers)
        {
            if(strcmp(layer_name, layer_properties.layerName) == 0)
            {
                layer_found = true;
                break;
            }
        }

        if(!layer_found)
        {
            return false;
        }
    }

    return true;
}

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

    //Enable validation layers
    const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation"};
    #ifdef NDEBUG
        const bool enable_validation_layers = false;
    #else
        const bool enable_validation_layers = true;
    #endif

    if(enable_validation_layers && !checkValidationLayerSupport(validation_layers))
    {
        std::cout << "Validation layers requested, but not available." << std::endl;
        return 1;
    }

    //Create Vulkan instance and query for extensions
    unsigned int extension_count = 0;
    if(!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr))
    {
        std::cout << "Could not get Vulkan instance extensions: " <<SDL_GetError() << std::endl;
        return 1;
    }
    std::vector<const char*> extensions = {};
    if(!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data()))
    {
        std::cout << "Could not get Vulkan instance extensions: " <<SDL_GetError() << std::endl;
        return 1;
    }
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    if(enable_validation_layers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    create_info.pApplicationInfo = &app_info;
    if(enable_validation_layers)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    }
    else
    {
        create_info.enabledLayerCount = 0;
    }

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
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}