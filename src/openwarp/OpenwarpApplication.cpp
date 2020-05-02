

#include "openwarp.hpp"
#include "vulkan_util.hpp"
#include "OpenwarpApplication.hpp"

using namespace Openwarp;

// The standard Khronos instance API validation layer,
// along with older validation layers for compatibility with
// Ubuntu 18.04's older Vulkan distribution.
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> fallbackValidationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

// Need the KHR_SWAPCHAIN_EXTENSION to use swapchains!
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

OpenwarpApplication::OpenwarpApplication(bool debug){
    std::cout << "Initializing GLFW...." << std::endl;
    glfwInit();
    std::cout << "Initializing application window...." << std::endl;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL API, please!
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    std::cout << "Application window successfully created...." << std::endl;
    is_debug = debug;
    initVulkan();
}

OpenwarpApplication::~OpenwarpApplication(){
    cleanupVulkan();
}

VkResult OpenwarpApplication::initVulkan(){
    instance.Init(validationLayers, fallbackValidationLayers);
    // Create the debug messenger.
    if (is_debug){
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
        debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugMessengerCreateInfo.flags = 0;
        debugMessengerCreateInfo.pfnUserCallback = OpenwarpUtils::debugCallback;

        if (OpenwarpUtils::CreateDebugUtilsMessengerEXT(instance.GetHandle(), &debugMessengerCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("Failed to setup debug messenger.");
        }
    }

    // Create the surface.
    surface.Init(instance, window);

    // Create the physical device.
    physicalDevice.Init(instance, surface, deviceExtensions);

    // Create the logical device.
    logicalDevice.Init(instance, physicalDevice, surface, deviceExtensions);

    return VK_SUCCESS;
}

VkResult OpenwarpApplication::cleanupVulkan(){
    logicalDevice.Destroy();
    physicalDevice.Destroy();
    if(is_debug){
        OpenwarpUtils::DestroyDebugUtilsMessengerEXT(instance.GetHandle(), debugMessenger, nullptr);
    }
    surface.Destroy();
    instance.Destroy();
    glfwDestroyWindow(window);
    return VK_SUCCESS;
}