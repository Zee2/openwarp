#include <openwarp.hpp>
#include <vulkan_util.hpp>

#include <algorithm>
#include <string>
#include <set>

using namespace Openwarp;

VkResult Surface::Init(VkInstance instance, GLFWwindow* window){
    associatedInstance = instance;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surfaceHandle) != VK_SUCCESS) {
        throw std::runtime_error("Error, failed to create window surface.");
    }
    else {
        std::cout << "Created GLFW window surface." << std::endl;
        return VK_SUCCESS;
    }
}
VkSurfaceKHR Surface::GetHandle() {
    if(surfaceHandle == VK_NULL_HANDLE) {
        throw std::runtime_error("Attempted to access surface handle when none exists.");
    } else {
        return surfaceHandle;
    }
}
VkResult Surface::Destroy(){
    std::cout<< "Destroying window surface." << std::endl;
    vkDestroySurfaceKHR(associatedInstance, surfaceHandle, nullptr);
    return VK_SUCCESS;
}

VkResult PhysicalDevice::Init(VkInstance instance, VkSurfaceKHR targetSurface, std::vector<const char*> requiredExtensions){
    associatedInstance = instance;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("Couldn't find any GPUs that have Vulkan support :(");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    for (const auto& device : devices) {
        if (isDeviceSuitable(device, targetSurface, requiredExtensions)) {
            physicalDeviceHandle = device;
            break;
        }
    }

    // If our physical device is STILL the null handle,
    // that means we didn't find any that were suitable.
    if (physicalDeviceHandle == VK_NULL_HANDLE) {
        throw std::runtime_error("None of the discovered GPUs are suitable.");
    }
    else {
        VkPhysicalDeviceProperties chosenDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDeviceHandle, &chosenDeviceProperties);
        std::cout << "System is choosing the following device as the physical device for the application:" << std::endl;
        std::cout << "\tName: " << chosenDeviceProperties.deviceName << std::endl;
        std::cout << "\tDriver version: " << chosenDeviceProperties.driverVersion << std::endl;
        std::cout << "\tVendor ID: " << chosenDeviceProperties.vendorID << std::endl;
    }
}

VkPhysicalDevice PhysicalDevice::GetHandle() {
    if(physicalDeviceHandle == VK_NULL_HANDLE) {
        throw std::runtime_error("Attempted to access physical device handle when none exists.");
    } else {
        return physicalDeviceHandle;
    }
}

VkResult PhysicalDevice::Destroy(){
    // This is no-op for PhysicalDevice.
    return VK_SUCCESS;
}

// Function for determining whether a particular device is suitable for the application.
bool PhysicalDevice::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, std::vector<const char*> requiredExtensions) {

    QueueFamilyIndices indices = Openwarp::OpenwarpUtils::findQueueFamilies(device, surface, true); // Fetches queue families.

    bool areExtensionsSupported = checkDeviceExtensionSupport(device, requiredExtensions);

    bool swapchainAdequate = false;
    if (areExtensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
        // For the swapchain to be sufficient, we need to have at least one supported format
        // and at least one supported presentation mode.
        swapchainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    // Perhaps in the future, we can rank physical device based on a suitability score.
    return indices.isComplete() && areExtensionsSupported && swapchainAdequate;
}

bool PhysicalDevice::checkDeviceExtensionSupport(VkPhysicalDevice device, std::vector<const char*> requestedExtensions) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    // Create a unique set of extension names from the vector of supported extensions.
    std::set<std::string> requiredExtensions(requestedExtensions.begin(), requestedExtensions.end());
    std::cout << "Detected " << availableExtensions.size() << " available device extensions" << std::endl;
    for (const auto& ext : availableExtensions) {
        // Erase the supported/available extension from the list of required extensions
        // (if we even require that extension!)
        requiredExtensions.erase(ext.extensionName);
    }

    // If we've "erased" all the required extensions, we're good to go.
    return requiredExtensions.empty();
}

SwapChainSupportDetails PhysicalDevice::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {

    // Query basic swapchain capabilities.
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // Query format count.
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount); // Formats vector needs to be big enough to fit all the formats!
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // Query supported presentation modes.
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}