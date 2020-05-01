#pragma once


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>
#include <iostream>

namespace Openwarp{
    class OpenwarpApplication;
    class OpenwarpUtils;
	class VulkanInstance;
    class Surface;
    class PhysicalDevice;
    class LogicalDevice;
    
    struct SwapChainSupportDetails;
    struct QueueFamilyIndices;
}

// Helper/container struct for keeping track of
// swapchain support details.
struct Openwarp::SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// Helper/container struct for keeping track
// of supported queue families for a particular device.
struct Openwarp::QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    // Checks whether all families have valid indices.
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class Openwarp::VulkanInstance{
public:
	VkResult Init(std::vector<const char*> requestedValidationLayers,std::vector<const char*> fallbackValidationLayers);
	VkResult Init();
    VkInstance GetHandle(){ return instance; }
	VkResult Destroy();
private:
    VkInstance instance; // The internal instance handle
	bool isDebugInstance = false;
	std::vector<const char*> activeValidationLayers;
    std::vector<const char*> getUsableValidationLayers(std::vector<const char*> requestedLayers, std::vector<const char*> fallbackLayers);
    // Takes a vector of layer names, and vector of VkLayerProperties and verifies that all queried layers are present in the
    // vector of available layers.
    bool checkIfAllLayersSupported(const std::vector<const char*> queriedLayers, const std::vector<VkLayerProperties> availableLayers);
    
};

class Openwarp::Surface{
public:
    VkResult Init(VkInstance instance, GLFWwindow* window);
    VkSurfaceKHR GetHandle();
    VkResult Destroy();

private:
    VkSurfaceKHR surfaceHandle = VK_NULL_HANDLE;
    VkInstance associatedInstance;
};

class Openwarp::PhysicalDevice{
public:
    VkResult Init(VkInstance instance, VkSurfaceKHR targetSurface, std::vector<const char*> requiredExtensions);
    VkPhysicalDevice GetHandle();
    VkResult Destroy();

private:
    VkPhysicalDevice physicalDeviceHandle = VK_NULL_HANDLE;
    VkInstance associatedInstance;
    // Function for determining whether a particular device is suitable for the application.
    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, std::vector<const char*> requiredExtensions);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device, std::vector<const char*> requestedExtensions);
    Openwarp::SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
};

class Openwarp::LogicalDevice{
public:
    VkResult Init(VkInstance instance);
    VkDevice GetHandle();
    VkResult Destroy();

private:
    VkDevice logicalDeviceHandle;
}