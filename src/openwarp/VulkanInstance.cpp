

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <cstring>
#include "openwarp.hpp"
#include "vulkan_util.hpp"

using namespace Openwarp;

VkResult VulkanInstance::Init(std::vector<const char*> requestedValidationLayers,std::vector<const char*> fallbackValidationLayers){
    std::cout << "Creating Vulkan instance (attempting to verify validation layers). " << std::endl;
    isDebugInstance = true;
    std::vector<const char*> usableValidationLayers = getUsableValidationLayers(requestedValidationLayers, fallbackValidationLayers);
    if (usableValidationLayers.size() == 0) {
        throw std::runtime_error("Validation layers were requested that are not available on this system!");
    } else {
        std::cout << "Validation layers actively running on this application:" << std::endl;
        for(auto const& layer : usableValidationLayers){
            std::cout << "\t" << layer << std::endl;
        }
    }
    activeValidationLayers = usableValidationLayers;
    return Init();
}
	
VkResult VulkanInstance::Init(){
    std::cout << "Creating Vulkan instance. " << std::endl;
    
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // This is special debug messenger specifically to watch over instance creation/deletion.
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
    if (isDebugInstance) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(activeValidationLayers.size());
        createInfo.ppEnabledLayerNames = activeValidationLayers.data();
        std::cout << "Creating debugMessengerCreateInfo" << std::endl;

        // Populate the debug messenger struct.
        debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugMessengerCreateInfo.flags = 0;
        debugMessengerCreateInfo.pfnUserCallback = Openwarp::OpenwarpUtils::debugCallback;
        
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)(&debugMessengerCreateInfo); // Attach our debug messenger createInfo to the instance createInfo.
    }
    else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    // Minimum set of extensions are those required by GLFW.
    auto requiredExtensions = Openwarp::OpenwarpUtils::getGlfwRequiredExtensions();
    
    // If we are debugging, add the debug extension.
    if(isDebugInstance){
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    std::cout << "Required instance extensions (" << requiredExtensions.size() << "):" << std::endl;
    for (const auto& extension : requiredExtensions) { std::cout << "\t" << extension << std::endl; }

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data());
    std::cout << "Supported instance extensions (" << extensionCount << "):" << std::endl;
    for (const auto& extension : supportedExtensions) { std::cout << "\t" << extension.extensionName << std::endl; }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        std::cout << "Error code: " << result << std::endl;
        throw std::runtime_error("Failed to create Vulkan instance.");
    }

    return VK_SUCCESS;
}

VkResult VulkanInstance::Destroy(){
    std::cout << "Tearing down Vulkan instance." << std::endl;
    vkDestroyInstance(instance, nullptr);
    return VK_SUCCESS;
}

std::vector<const char*> VulkanInstance::getUsableValidationLayers(std::vector<const char*> requestedLayers, std::vector<const char*> fallbackLayers) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    std::cout << "Available validation layers (" << layerCount << ") on this system: " << std::endl;
    for (const auto& layer : availableLayers) { std::cout << "\t" << layer.layerName << std::endl; }


    // See if all of our "preferred" validation layers are available.
    bool preferredAvailable = checkIfAllLayersSupported(requestedLayers, availableLayers);

    
    if(!preferredAvailable){
        // If our preferred layers are not available, check the fallback layers.
        bool fallbackAvailable = checkIfAllLayersSupported(fallbackLayers, availableLayers);

        // If even the fallback layers are not available, we fail the check entirely.
        if(!fallbackAvailable){
            return std::vector<const char*>(0);
        } else {
            // If the fallback is available, we copy 
            // the fallback layers vector to the "active validation layers"
            // vector for future bookkeeping.
            std::cout << "WARNING: Using fallback validation layer! This is probably because you have " <<
                            "an old Vulkan SDK without the newest validation layers." << std::endl;
            return fallbackLayers;
        }
    } else {

        // Our preferred layers are available.
        return requestedLayers;
    }
}
// Takes a vector of layer names, and vector of VkLayerProperties and verifies that all queried layers are present in the
// vector of available layers.
bool VulkanInstance::checkIfAllLayersSupported(const std::vector<const char*> queriedLayers, const std::vector<VkLayerProperties> availableLayers){
    for (const char* queriedLayer : queriedLayers) {
        bool layerFoundFlag = false;
        for (const auto& availableLayer : availableLayers) {
            if (strcmp(queriedLayer, availableLayer.layerName) == 0) {
                layerFoundFlag = true;
                break;
            }
        }
        // If we failed to find even one of the layers, we have failed.
        if(layerFoundFlag == false){
            return false;
        }
    }
    return true;
}


