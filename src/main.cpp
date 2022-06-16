#define SDL_MAIN_HANDLED
#define VK_USE_PLATFORM_WAYLAND_KHR

#include <iostream>
#include <vector>
#include <set>
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

    if(message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT 
        || message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        std::cout << "WARNING/ERROR" << std::endl;
    }

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
            physical_device = VK_NULL_HANDLE;
            graphics_family = -1;
            present_family = -1;
            device = {};
            graphics_queue = {};
            surface = {};
       }
       ~Renderer()
       {
            if(enable_validation_layers)
            {
                DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
            }
            vkDestroyDevice(device, nullptr);
            vkDestroySurfaceKHR(instance, surface, nullptr);
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
        VkPhysicalDevice physical_device;
        uint32_t graphics_family;
        uint32_t present_family;
        VkDevice device;
        VkQueue graphics_queue;
        VkSurfaceKHR surface;
        VkQueue present_queue;

        const int screen_width = 1280;
        const int screen_height = 720;

        bool initAndCreateSDLWindow();
        bool createInstance(bool, std::vector<const char*>, const std::vector<const char*>);
        bool setupDebugMessenger();
        bool enableValidationLayer();
        bool createInstance();
        bool queryExtensions();
        bool initVulkan();
        bool pickPhysicalDevice();
        bool isDeviceSuitable(VkPhysicalDevice);
        bool findQueueFamilies();
        bool createLogicalDevice();
        bool createSurface();

};

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT 
                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT 
                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT 
                            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT 
                            |VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
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

bool Renderer::createSurface()
{

    // VkXlibSurfaceCreateInfoKHR create_info = {};
    // create_info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    // create_info.window = 
    if(!SDL_Vulkan_CreateSurface(sdl_window, instance, &surface))
    {
        std::cout << "Failed to create SDL Vulkan surface! " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

bool Renderer::queryExtensions()
{
    unsigned int extension_count = 0;
    if(!SDL_Vulkan_GetInstanceExtensions(sdl_window, &extension_count, nullptr))
    {
        std::cout << "Could not get Vulkan instance extensions: " << SDL_GetError() << std::endl;
        return false;
    }
    extensions.resize(extension_count);
    if(!SDL_Vulkan_GetInstanceExtensions(sdl_window, &extension_count, extensions.data()))
    {
        std::cout << "Could not get Vulkan instance extensions: " << SDL_GetError() << std::endl;
        return false;
    }
    //extensions.push_back(VK_KHR_surface);
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

bool Renderer::createLogicalDevice()
{
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos = {};
    std::set<uint32_t> unique_queue_families = {graphics_family, present_family};
    const float queue_priority = 1.0f;
    for(uint32_t queue_family : unique_queue_families)
    {
        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }


    VkPhysicalDeviceFeatures device_features = {};

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = 0;

    if(enable_validation_layers)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    }
    else
    {
        create_info.enabledLayerCount = 0;
    }

    if(vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS)
    {
        std::cout << "Failed to create logical device!" << std::endl;
        return false;
    }

    vkGetDeviceQueue(device, graphics_family, 0, &graphics_queue);
    vkGetDeviceQueue(device, present_family, 0, &present_queue);

    return true;
}

bool Renderer::findQueueFamilies()
{
    uint32_t queue_families_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, nullptr);
    if(queue_families_count == 0)
    {
        std::cout << "Physical device " << physical_device << " doesn't have any queue families!" << std::endl;
        return false;
    }

    std::vector<VkQueueFamilyProperties> queue_families(queue_families_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, &queue_families[0]);
    int i = 0;
    for(const auto& queue_family : queue_families)
    {
        if((queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queue_family.queueCount > 0))
        {
            graphics_family = i;
        }
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
        if(present_support)
        {
            present_family = i;
        }
        i++;
    }
    if(graphics_family == -1)
    {
        std::cout << "Could not find appropriate graphics queue family!" << std::endl;
        return false;
    }
    if(present_family == -1)
    {
        std::cout << "Could not find appropriate present queue family!" << std::endl;
        return false;
    }

    return true;
}

bool Renderer::isDeviceSuitable(VkPhysicalDevice device)
{
    // VkPhysicalDeviceProperties device_properties = {};
    // vkGetPhysicalDeviceProperties(device, &device_properties);

    // VkPhysicalDeviceFeatures device_features = {};
    // vkGetPhysicalDeviceFeatures(device, &device_features);

    // return device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
    //     device_features.geometryShader;
    
    //For Hello Triangle, GPU only needs to have Vulkan support
    return true;
}

bool Renderer::pickPhysicalDevice()
{
    //Find all physical GPUs on host
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if(device_count == 0)
    {
        std::cout << "Failed to find GPUs with Vulkan support!" << std::endl;
        return false;
    }
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    //Evaluate GPUs
    for(const auto& device: devices)
    {
        if(isDeviceSuitable(device))
        {
            physical_device = device;
            break;
        }
    }

    if(physical_device == VK_NULL_HANDLE)
    {
        std::cout << "Filed to find a suitable GPU!" << std::endl;
        return false;
    }
 
    return true;
}

bool Renderer::initVulkan()
{
    bool result = initAndCreateSDLWindow();
    if(!result)
    {
        return false;
    }

    SDL_ShowWindow(sdl_window);

    result = enableValidationLayer();
    if(!result)
    {
        return false;
    }

    result = queryExtensions();
    if(!result)
    {
        return false;
    }

    result = createInstance();
    if(!result)
    {
        return false;
    }

    result = setupDebugMessenger();
    if(!result)
    {
        return false;
    }

    result = createSurface();
    if(!result)
    {
        return false;
    }

    result = pickPhysicalDevice();
    if(!result)
    {
        return false;
    }

    result = findQueueFamilies();
    if(!result)
    {
        return false;
    }

    result = createLogicalDevice();
    if(!result)
    {
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    std::cout << "Hello World!" << std::endl;

    Renderer renderer;
    bool result = renderer.initVulkan();
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