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

VkResult Swapchain::Init(uint32_t width, uint32_t height,
                            const Openwarp::VulkanInstance& instance,
                            const Openwarp::LogicalDevice& logicalDevice,
                            const Openwarp::Surface& surface,
                            const Openwarp::PhysicalDevice& physicalDevice){

    Openwarp::SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice.GetHandle(), surface.GetHandle());

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats); // Image/texture format, etc
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, VK_PRESENT_MODE_FIFO_KHR); // Vsync, etc
    VkExtent2D requestedExtent = chooseSwapExtent(width, height, swapChainSupport.capabilities); // Size/resolution, etc

    // Good practice to request one more swapchain image than the image, to avoid unnecessary stalling.
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    // If somehow, minImageCount + 1 is actually greater than the max image count, we'll fall back to
    // whatever the max image count actually is.
    if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        std::cout << "WARNING: minImageCount+1 is greater than maxImageCount; falling back to maxImageCount ("
            << swapChainSupport.capabilities.maxImageCount << ")" << std::endl;
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // Time to create the swapchain object.
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface.GetHandle(); // Attach the surface we should have already acquired through GLFW.
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = requestedExtent;
    createInfo.imageArrayLayers = 1; // VR, hmmmmmm.....
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // We assume these are populated because we should have already checked the queue families of this device.
    const Openwarp::QueueFamilyIndices& indices = physicalDevice.GetFamilies();
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    // If the graphics queue we're using is the same queue as the present queue,
    // we can use exclusive sharing mode; however, if the two queues are actually
    // different, we'll have to use concurrent sharing mode so that the image can
    // be concurrently owned by both queues.
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Concurrent sharing between the two queues
        createInfo.queueFamilyIndexCount = 2; // Indicates that two queues will be sharing this swapchain
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Exclusive sharing, same queue
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // No transformation.

    // Sets the alpha composite settings for interacting with the OS.
    // This setting ignores alpha and makes the surface opaque.
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode; // Sets the present mode (vsync/etc)
    createInfo.clipped = VK_TRUE; // Performance optimization to discard window clipped pixels.

    // For now, oldSwapchain will be null.
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(logicalDevice.GetHandle(), &createInfo, nullptr, &swapchainHandle) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain.");
    }
    else {
        std::cout << "Successfully created swapchain" << std::endl;
    }

    // Re-query the number of images that actually ended up getting created.
    vkGetSwapchainImagesKHR(logicalDevice.GetHandle(), swapchainHandle, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(logicalDevice.GetHandle(), swapchainHandle, &imageCount, images.data());

    // Save the format and extent of our new swapchain for later.
    format = surfaceFormat.format;
    extent = extent;

    std::cout << "Acquired handles to created images in swapchain (" << imageCount << " created)" << std::endl;
}

VkExtent2D Swapchain::chooseSwapExtent(uint32_t width, uint32_t height,
                                        const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        // Calculate some reasonable dimensions to use as our swap extent.
        // We need to clamp this so that we don't accidentally exceed the
        // swapchain's maximum or minimum extent capabilities.
        VkExtent2D actualExtent = { width, height };
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }
}

// Chooses an appropriate swap present mode from the list of available presentation modes.
// An ideal presentation mode can be specified optionally.
VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes,
                                        VkPresentModeKHR idealMode,
                                        VkPresentModeKHR fallbackMode,
                                        bool verbose/* = true*/) {
    if (verbose) {
        std::cout << "Available swapchain presentation modes (" << availablePresentModes.size() << "):" << std::endl;
        for (const auto& availablePresentMode : availablePresentModes) {
            std::cout << "\t" << presentModeToString(availablePresentMode) << std::endl;
        }
    }
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == idealMode) {
            if (verbose) { std::cout << "Requested presentation mode satisfied: " << presentModeToString(availablePresentMode) << std::endl; }
            return availablePresentMode;
        }
    }
    if (verbose) { std::cout << "Using default presentation mode: " << presentModeToString(fallbackMode) << std::endl; }
    return fallbackMode;
}

VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
            && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat; // We found our desired swapchain format.
        }
    }

    // If we've exhausted all options, the first format is probably our best bet.
    return availableFormats[0];
}

Openwarp::SwapChainSupportDetails Swapchain::querySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) {

    // Query basic swapchain capabilities.
    Openwarp::SwapChainSupportDetails details;
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

std::string Swapchain::presentModeToString(const VkPresentModeKHR& mode) {
    switch (mode) {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
            return "Immediate";
        case VK_PRESENT_MODE_MAILBOX_KHR:
            return "Mailbox";
        case VK_PRESENT_MODE_FIFO_KHR:
            return "FIFO";
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
            return "Relaxed FIFO";
        case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
            return "Shared Demand Refresh";
        case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
            return "Shared Continuous Refresh";
        default:
            return "Unknown presentation mode: " + std::to_string(mode);
    }
}