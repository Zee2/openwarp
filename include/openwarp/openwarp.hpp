#pragma once


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>
#include <iostream>

#define GET_IF_VALID(x) if(x != VK_NULL_HANDLE) { return x; } else { throw std::runtime_error("Attempted to access " #x " handle when none exists."); }

namespace Openwarp{
    class OpenwarpApplication;
    class OpenwarpUtils;

    // Vulkan containers.
	class VulkanInstance;
    class Surface;
    class PhysicalDevice;
    class LogicalDevice;
    class Swapchain;
    
    // Helper structs.
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
    const VkInstance& GetHandle() const { GET_IF_VALID(instance) }
    bool GetDebug() const { return isDebugInstance; }
    const std::vector<const char*>& GetValidationLayers() const {return activeValidationLayers; }
	VkResult Destroy();
private:
    VkInstance instance = VK_NULL_HANDLE; // The internal instance handle
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
    const VkSurfaceKHR& GetHandle() const { GET_IF_VALID(surfaceHandle) }
    VkResult Destroy();

private:
    VkSurfaceKHR surfaceHandle = VK_NULL_HANDLE;
    VkInstance associatedInstance;
};

class Openwarp::PhysicalDevice{
public:
    VkResult Init(const Openwarp::VulkanInstance& instance,
                    const Openwarp::Surface& targetSurface,
                    const std::vector<const char*>& requiredExtensions);
    const VkPhysicalDevice& GetHandle() const { GET_IF_VALID(physicalDeviceHandle) }
    const QueueFamilyIndices& GetFamilies() const { return familyIndices; }
    VkResult Destroy();

private:
    VkPhysicalDevice physicalDeviceHandle = VK_NULL_HANDLE;
    VkInstance associatedInstance;
    QueueFamilyIndices familyIndices;
    // Function for determining whether a particular device is suitable for the application.
    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, std::vector<const char*> requiredExtensions);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device, std::vector<const char*> requestedExtensions);
    Openwarp::SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
};

class Openwarp::LogicalDevice{
public:
    VkResult Init(const Openwarp::VulkanInstance& instance,
                    const Openwarp::PhysicalDevice& physicalDevice,
                    const Openwarp::Surface& surface,
                    std::vector<const char*> deviceExtensions);

    const VkDevice& GetHandle() const { GET_IF_VALID(logicalDeviceHandle) }
    const VkQueue& GetGraphicsQueue() const { GET_IF_VALID(graphicsQueue) }
    const VkQueue& GetPresentQueue() const { GET_IF_VALID(presentQueue) }
    VkResult Destroy();

private:
    VkDevice logicalDeviceHandle;
    VkQueue graphicsQueue;
    VkQueue presentQueue; 
};

class Openwarp::Swapchain{
public:
    VkResult Init(uint32_t width, uint32_t height,
                    const Openwarp::VulkanInstance& instance,
                    const Openwarp::LogicalDevice& logicalDevice,
                    const Openwarp::Surface& surface,
                    const Openwarp::PhysicalDevice& physicalDevice);
    VkSwapchainKHR GetHandle() { GET_IF_VALID(swapchainHandle) }
    VkResult Destroy();

private:
    VkSwapchainKHR swapchainHandle;
    std::vector<VkImage> images;
    VkExtent2D extent;
    VkFormat format;
    VkExtent2D chooseSwapExtent(uint32_t width, uint32_t height,
                                const VkSurfaceCapabilitiesKHR& capabilities);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes,
                                        VkPresentModeKHR idealMode = VK_PRESENT_MODE_FIFO_KHR,
                                        VkPresentModeKHR fallbackMode = VK_PRESENT_MODE_FIFO_KHR,
                                        bool verbose = true);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    Openwarp::SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
    std::string presentModeToString(const VkPresentModeKHR& mode);
};