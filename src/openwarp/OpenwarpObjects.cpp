#include <openwarp.hpp>
#include <vulkan_util.hpp>

#include <algorithm>
#include <string>
#include <set>

using namespace Openwarp;

VkResult Surface::Init(const Openwarp::VulkanInstance& instance, GLFWwindow* window){
    associatedInstance = instance.GetHandle();
    if (glfwCreateWindowSurface(associatedInstance, window, nullptr, &surfaceHandle) != VK_SUCCESS) {
        throw std::runtime_error("Error, failed to create window surface.");
    }
    else {
        std::cout << "Created GLFW window surface." << std::endl;
        return VK_SUCCESS;
    }
}

VkResult Surface::Destroy(){
    std::cout<< "Destroying window surface." << std::endl;
    vkDestroySurfaceKHR(associatedInstance, surfaceHandle, nullptr);
    return VK_SUCCESS;
}

VkResult PhysicalDevice::Init(const Openwarp::VulkanInstance& instance,
                                const Openwarp::Surface& targetSurface,
                                const std::vector<const char*>& requiredExtensions){
    associatedInstance = instance.GetHandle();

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(associatedInstance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("Couldn't find any GPUs that have Vulkan support :(");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(associatedInstance, &deviceCount, devices.data());
    for (const auto& device : devices) {
        if (isDeviceSuitable(device, targetSurface.GetHandle(), requiredExtensions)) {
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

    // Record the queue families for later use.
    familyIndices = Openwarp::OpenwarpUtils::findQueueFamilies(physicalDeviceHandle, targetSurface.GetHandle(), false);

    return VK_SUCCESS;
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

VkResult LogicalDevice::Init(const Openwarp::VulkanInstance& instance,
                                const Openwarp::PhysicalDevice& physicalDevice,
                                const Openwarp::Surface& surface,
                                std::vector<const char*> deviceExtensions){
    
    std::cout << "Creating/allocating logical device object. " << std::endl;

    // Get the queue families from the physical device.
    Openwarp::QueueFamilyIndices indices = physicalDevice.GetFamilies();

    // We're going to create several queues; so we make a vector of createInfos.
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    // This unique set of queue families will serve as our way of iterating over
    // all of the queues that are supported/required, which we need to create.
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (auto queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily; // Set the family index to the current queue family.
        queueCreateInfo.queueCount = 1; // Just one queue.
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo); // Add this struct to our vector of structs.
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    VkDeviceCreateInfo logicalDeviceCreateInfo{};
    logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logicalDeviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data(); // Link our vector of queue infos to the logical device creation struct.
    logicalDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()); // We have several device queues to.
    logicalDeviceCreateInfo.pEnabledFeatures = &deviceFeatures; // Link our deviceFeatures struct to the logical device creation struct.

    // Populate the struct with the device-specific extensions we are using.
    logicalDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    logicalDeviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // Note: modern implementations of Vulkan make no distinction between the device-level and instance-level validation layers.
    // Therefore, VkDeviceCreateInfo.enabledLayerCount and VkDeviceCreateInfo.ppEnabledLayerNames are ignored.
    // It is good practice to populate these fields regardless.

    if (instance.GetDebug()) { // If we are currently using validation layers at the instance level...
        logicalDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(instance.GetValidationLayers().size()); // Pass those layers to the device.
        logicalDeviceCreateInfo.ppEnabledLayerNames = instance.GetValidationLayers().data();
    }
    else { // If we aren't using any instance validation layers, we won't be using any device layers either.
        logicalDeviceCreateInfo.enabledLayerCount = 0;
    }

    // Instantiate the logical device object.
    if (vkCreateDevice(physicalDevice.GetHandle(), &logicalDeviceCreateInfo, nullptr, &logicalDeviceHandle) != VK_SUCCESS) {
        throw std::runtime_error("Error: failed to create logical device.");
    }

    // Acquire a handle to the graphics command queue of this device.
    vkGetDeviceQueue(logicalDeviceHandle, indices.graphicsFamily.value(), 0, &graphicsQueue);
    // Acquire a handle to the presentation command queue of this device.
    vkGetDeviceQueue(logicalDeviceHandle, indices.presentFamily.value(), 0, &presentQueue);

    std::cout << "Logical device object created and queue handles acquired." << std::endl;

    return VK_SUCCESS;
}

VkResult LogicalDevice::Destroy(){
    vkDestroyDevice(logicalDeviceHandle, nullptr);
    return VK_SUCCESS;
}