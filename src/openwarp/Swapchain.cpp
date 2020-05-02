

#include <openwarp.hpp>
#include <algorithm>
using namespace Openwarp;

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
    
    return VK_SUCCESS;
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