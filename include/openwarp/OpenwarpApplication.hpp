#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <cstdint>

#include <openwarp.hpp>
#include <vulkan_util.hpp>

class Openwarp::OpenwarpApplication{

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    public:
        OpenwarpApplication(bool debug);
        ~OpenwarpApplication();

        void Run();

    private:

        // Application metadata
        bool is_debug;

        // GLFW resources
        GLFWwindow* window;

        // Vulkan instance and device creation/management objects.
        Openwarp::VulkanInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger; // Vulkan object that handles debug callbacks
        Openwarp::PhysicalDevice physicalDevice; // The graphics card.
        VkDevice logicalDevice; // The logical device. (We'll just use one.)
        VkQueue graphicsQueue; // Device queues are automatically cleaned up when the logical device is cleaned up.

        // Vulkan window/surface/swapchain creation objects.
        Openwarp::Surface surface;
        VkQueue presentQueue; // Device queue for presenting to the surface.
        VkSwapchainKHR swapchain; // Handle to the overall swapchain object
        VkFormat swapchainImageFormat;
        VkExtent2D swapchainExtent;

        // Swapchain resources
        std::vector<VkImage> swapchainImages; // The swapchain images themselves.
        std::vector<VkImageView> swapchainImageViews; // Views into the above swapchain images.

        // Pipeline resources
        VkRenderPass renderPass; // Single render pass
        VkPipelineLayout pipelineLayout; // Graphics pipeline layout, contains uniform data
        VkPipeline graphicsPipeline; // The actual graphics pipeline itself
        
        // Framebuffer resources
        std::vector<VkFramebuffer> swapchainFramebuffers;

        // Command resources
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers; // Command buffers are auto-freed when the commandPool is freed.

        // Synchronization resources
        std::vector<VkSemaphore> imageAvailableSemaphores; // Is the image available for rendering?
        std::vector<VkSemaphore> renderFinishedSemaphores; // Are we done rendering to the image?
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight; // Keep track of which images are actually currently being rendered to.
        size_t currentFrame = 0;
        float lastFrameTime = 0;

        VkResult initVulkan();
        VkResult cleanupVulkan();
};