#define SDL_MAIN_HANDLED
#define VK_USE_PLATFORM_WAYLAND_KHR

#include <iostream>
#include <vector>
#include <set>
#include <cstring>
#include <optional>
#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include <fstream>
#include <vulkan/vulkan.h>
#include "SDL.h"
#include "SDL_vulkan.h"

struct QueueFamilyIndices
{
    //Support is required by the spec, so it's okay to set these to 0
    uint32_t graphics_family = 0;
    uint32_t present_family = 0;
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats = {};
    std::vector<VkPresentModeKHR> present_modes = {};
};

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
            // graphics_family = -1;
            // present_family = -1;
            device = {};
            graphics_queue = {};
            surface = {};
            swap_chain = {};
            swap_chain_images = {};
            swap_chain_extent = {};
            swap_chain_image_format = {};
            swap_chain_image_views = {};
       }
       ~Renderer()
       {
            if(enable_validation_layers)
            {
                DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
            }
            for(auto image_view : swap_chain_image_views)
            {
                vkDestroyImageView(device, image_view, nullptr);
            }
            vkDestroySwapchainKHR(device, swap_chain, nullptr);
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
        QueueFamilyIndices indices;
        // uint32_t graphics_family;
        // uint32_t present_family;
        VkDevice device;
        VkQueue graphics_queue;
        VkSurfaceKHR surface;
        VkQueue present_queue;
        const std::vector<const char*> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        SwapChainSupportDetails swap_chain_support;
        VkSwapchainKHR swap_chain;
        std::vector<VkImage> swap_chain_images;
        VkExtent2D swap_chain_extent;
        VkFormat swap_chain_image_format;
        std::vector<VkImageView> swap_chain_image_views;

        const int window_width = 1280;
        const int window_height = 720;

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
        bool checkDeviceExtensionSupport(VkPhysicalDevice);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR&);
        bool createSwapChain();
        bool createImageViews();
        bool createGraphicsPipeline();
        VkShaderModule createShaderModule(const std::vector<char>&, bool*);

};

VkShaderModule Renderer::createShaderModule(const std::vector<char>& code, bool* result)
{
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shader_module = {};
    if(vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
    {
        std::cout << "Failed to create shader module!" << std::endl;
        *result = false;
    }

    *result = true;
    return shader_module;
}

static std::vector<char> readFile(const std::string& file_name, bool* result)
{
    std::ifstream file(file_name, std::ios::ate | std::ios::binary);

    if(!file.is_open())
    {
        std::cout << "Failed to open file: " << file_name << std::endl;
        *result = false;
    }

    size_t file_size = (size_t) file.tellg();
    std::vector<char> buffer(file_size);
    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();
    *result = true;
    return buffer;
}

bool Renderer::createGraphicsPipeline()
{
    bool result = false;
    auto vert_shader_code = readFile("shaderssss/vert.spv", &result);
    if(!result)
    {
        return false;
    }
    auto frag_shader_code = readFile("shaders/frag.spv", &result);
    if(!result)
    {
        return false;
    }

    VkShaderModule vert_shader_module = createShaderModule(vert_shader_code, &result);
    if(!result)
    {
        return false;
    }
    VkShaderModule frag_shader_module = createShaderModule(frag_shader_code, &result);
    if(!result)
    {
        return false;
    }

    vkDestroyShaderModule(device, frag_shader_module, nullptr);
    vkDestroyShaderModule(device, vert_shader_module, nullptr);

    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages = {vert_shader_stage_info, frag_shader_stage_info};

    // We are hardcoding vertex data in the shader
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.pVertexBindingDescriptions = nullptr;
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swap_chain_extent.width;
    viewport.height = (float) swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 0.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swap_chain_extent;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    return true;
}

bool Renderer::createImageViews()
{
    swap_chain_image_views.resize(swap_chain_images.size());

    for(size_t i = 0; i < swap_chain_images.size(); i++)
    {
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swap_chain_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swap_chain_image_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if(vkCreateImageView(device, &create_info, nullptr, &swap_chain_image_views[i]) != VK_SUCCESS)
        {
            std::cout << "Failed to create image views!" << std::endl;
            return false;
        }
    }

    return true;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
    for(const auto& available_format : available_formats)
    {
        if(available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return available_format;
        }
    }

    return available_formats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
    for(const auto& available_present_mode : available_present_modes)
    {
        if(available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return available_present_mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width = 0;
        int height = 0;
        SDL_Vulkan_GetDrawableSize(sdl_window, &width, &height);

        VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width,
                                swap_chain_support.capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height,
                                swap_chain_support.capabilities.maxImageExtent.height);

        return actual_extent;

    }

}

SwapChainSupportDetails Renderer::querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
    if(format_count != 0)
    {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
    }

    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
    if(present_mode_count != 0)
    {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
    }

    return details;
}

bool Renderer::createSwapChain()
{
    swap_chain_support = querySwapChainSupport(physical_device);

    VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats);
    VkPresentModeKHR present_mode = chooseSwapPresentMode(swap_chain_support.present_modes);
    swap_chain_extent = chooseSwapExtent(swap_chain_support.capabilities);
    swap_chain_image_format = surface_format.format;

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if(swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
    {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = swap_chain_extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queue_family_indices[] = {indices.graphics_family, indices.present_family};
    if(indices.graphics_family != indices.present_family)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }
    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if(vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain) != VK_SUCCESS)
    {
        std::cout << "Failed to create swap chain!" << std::endl;
        return false;
    }

    //Retrieve swap chain images
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
    swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_images.data());

    return true;
}

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

    sdl_window = SDL_CreateWindow("Vulkan Intro", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
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
    std::set<uint32_t> unique_queue_families = {indices.graphics_family, indices.present_family};
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
    create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    create_info.ppEnabledExtensionNames = device_extensions.data();

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

    vkGetDeviceQueue(device, indices.graphics_family, 0, &graphics_queue);
    vkGetDeviceQueue(device, indices.present_family, 0, &present_queue);

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
            indices.graphics_family = i;
        }
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
        if(present_support)
        {
            indices.present_family = i;
        }
        i++;
    }

    return true;
}

bool Renderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

    for(const auto& extension : available_extensions)
    {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

bool Renderer::isDeviceSuitable(VkPhysicalDevice device)
{
    //For Hello Triangle, GPU only needs to have Vulkan support
    // VkPhysicalDeviceProperties device_properties = {};
    // vkGetPhysicalDeviceProperties(device, &device_properties);

    // VkPhysicalDeviceFeatures device_features = {};
    // vkGetPhysicalDeviceFeatures(device, &device_features);

    // return device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
    //     device_features.geometryShader;
    //QueueFamilyIndices qindices = findQueueFamilies(device);
    bool extensions_supported = checkDeviceExtensionSupport(device);
    bool swap_chain_adequate = false;
    if(extensions_supported)
    {
        SwapChainSupportDetails details = querySwapChainSupport(device);
        swap_chain_adequate = !details.formats.empty() && !details.present_modes.empty();
    }
    return extensions_supported && swap_chain_adequate;
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

    result = createSwapChain();
    if(!result)
    {
        return false;
    }

    result = createImageViews();
    if(!result)
    {
        return false;
    }
    result = createGraphicsPipeline();
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