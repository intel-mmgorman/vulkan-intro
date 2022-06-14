#define SDL_MAIN_HANDLED

#include <iostream>
#include <vector>
#include <cstring>
#include <vulkan/vulkan.h>
#include "SDL.h"
#include "SDL_vulkan.h"


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data) {

    std::cerr << "validation layer: " << p_callback_data->pMessage << std::endl;

    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

class Renderer
{
    public:
        Renderer()
       {
            sdl_window = {};
            instance = {};
            validation_layers = { "VK_LAYER_KHRONOS_validation"};
            extensions = {};
            debug_messenger = {};
            enable_validation_layers = false;
       }
       ~Renderer()
       {
            if(enable_validation_layers)
            {
                DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
            }
            vkDestroyInstance(instance, nullptr);
            SDL_DestroyWindow(sdl_window);
       }
        //SDL
        SDL_Window *sdl_window;
        //Vulkan
        VkInstance instance;
        std::vector<const char*> validation_layers;
        std::vector<const char*> extensions;
        VkDebugUtilsMessengerEXT debug_messenger;
        bool enable_validation_layers;

        const int screen_width = 1280;
        const int screen_height = 720;

        bool initAndCreateSDLWindow();
        bool createInstance(bool enable_validation_layers, std::vector<const char*> extensions, const std::vector<const char*> validation_layers);
        bool setupDebugMessenger();
        bool enableValidationLayer();
        bool createInstance();
        bool queryExtensions();

};

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debugCallback;
    create_info.pUserData = nullptr; //Optional
}

bool Renderer::setupDebugMessenger()
{
    if(enable_validation_layers)
    {
        VkDebugUtilsMessengerCreateInfoEXT create_info;
        populateDebugMessengerCreateInfo(create_info);
        if(CreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger))
        {
            std::cout << "Failed to set up debug messenger. " << std::endl;
            return false;
        }
    }
    return true;
}

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

bool Renderer::enableValidationLayer()
{
    #ifdef NDEBUG
        enable_validation_layers = false;
    #else
        enable_validation_layers = true;
    #endif

    if(enable_validation_layers && !checkValidationLayerSupport(validation_layers))
    {
        std::cout << "Validation layers requested, but not available." << std::endl;
        return false;
    }
    return true;
}

bool Renderer::queryExtensions()
{
    unsigned int extension_count = 0;
    if(!SDL_Vulkan_GetInstanceExtensions(sdl_window, &extension_count, nullptr))
    {
        std::cout << "Could not get Vulkan instance extensions: " <<SDL_GetError() << std::endl;
        return false;
    }
    if(!SDL_Vulkan_GetInstanceExtensions(sdl_window, &extension_count, extensions.data()))
    {
        std::cout << "Could not get Vulkan instance extensions: " <<SDL_GetError() << std::endl;
        return false;
    }
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    if(enable_validation_layers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return true;
}

bool Renderer::createInstance()
{
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_instance_info = {};
    create_instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_instance_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_instance_info.ppEnabledExtensionNames = extensions.data();
    create_instance_info.pApplicationInfo = &app_info;


    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
    if(enable_validation_layers)
    {
        create_instance_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_instance_info.ppEnabledLayerNames = validation_layers.data();
        populateDebugMessengerCreateInfo(debug_create_info);
        create_instance_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_create_info;
    }
    else
    {
        create_instance_info.enabledLayerCount = 0;
        create_instance_info.pNext = nullptr;
    }

    if(vkCreateInstance(&create_instance_info, nullptr, &instance) != VK_SUCCESS)
    {
        std::cout << "Failed to create Vulkan instance." << std::endl;
        return false;
    }

    return true;
}

bool Renderer::initAndCreateSDLWindow()
{
    //SDL open window
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "SDL could not initialize! Error: " << SDL_GetError() << std::endl;
        return false;
    }

    sdl_window = SDL_CreateWindow("Vulkan Intro", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    if(sdl_window == nullptr)
    {
        std::cout << "Could not create window: " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    std::cout << "Hello World!" << std::endl;

    Renderer renderer;
    bool result = renderer.initAndCreateSDLWindow();
    if(!result)
    {
        return 1;
    }

    SDL_ShowWindow(renderer.sdl_window);

    //Enable validation layers
    result = renderer.enableValidationLayer();
    if(!result)
    {
        return 1;
    }

    //Create Vulkan instance and query for extensions
    result = renderer.queryExtensions();
    if(!result)
    {
        return 1;
    }

    result = renderer.createInstance();
    if(!result)
    {
        return 1;
    }

    result = renderer.setupDebugMessenger();
    if(!result)
    {
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
    
    SDL_Quit();

}