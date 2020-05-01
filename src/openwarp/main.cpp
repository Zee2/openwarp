

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

#include <vulkan_util.hpp>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

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

// Will enable/disable API validation depending on build configuration.
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// Wrapper function that loads the function address of vkCreateDebugUtilsMessengerEXT for us.
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator,
                                        VkDebugUtilsMessengerEXT* pDebugMessenger) {

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

std::string presentModeToString(VkPresentModeKHR mode) {
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

// Another wrapper/proxy function that loads the vkDestroyDebugUtilsMessengerEXT
// (the destructor to the object that is created in the above proxy function)
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                    VkDebugUtilsMessengerEXT debugMessenger,
                                    const VkAllocationCallbacks* pAllocator) {

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:

    // Helper/container struct for keeping track
    // of supported queue families for a particular device.
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        // Checks whether all families have valid indices.
        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    // Helper/container struct for keeping track of
    // swapchain support details.
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    GLFWwindow* window;

    // Vulkan instance and device creation/management objects.
    VkInstance instance;
    std::vector<const char*> activeValidationLayers;
    VkDebugUtilsMessengerEXT debugMessenger; // Vulkan object that handles debug callbacks
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // The graphics card.
    VkDevice logicalDevice; // The logical device. (We'll just use one.)
    VkQueue graphicsQueue; // Device queues are automatically cleaned up when the logical device is cleaned up.

    // Vulkan window/surface/swapchain creation objects.
    VkSurfaceKHR surface;
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

    void initWindow() {
        std::cout << "Initializing GLFW...." << std::endl;
        glfwInit();
        std::cout << "Initializing application window...." << std::endl;
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL API, please!
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        std::cout << "Application window successfully created...." << std::endl;
    }

    void initVulkan() {

        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapchain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSynchronizationObjects();
    }

    void createSynchronizationObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        // Initially all frames are not being drawn to,
        // so all imagesInFlight fences are initialized to "no fence".
        imagesInFlight.resize(swapchainImages.size(), VK_NULL_HANDLE);

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Create the fence in the "signalled" state

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                    vkCreateFence(logicalDevice, &fenceInfo,     nullptr, &inFlightFences[i])           != VK_SUCCESS){
                throw std::runtime_error("Failed to create synchronization objects.");
            }
        }
        
    }

    void createCommandBuffers() {
        // We need to explicitly create one command buffer per framebuffer.
        commandBuffers.resize(swapchainFramebuffers.size());

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool;
        // PRIMARY command buffers cannot be called by other command buffers, while
        // SECOND command buffers can be called by others (used to reuse commands)
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers!");
        }
        else {
            std::cout << "Successfully created command buffers." << std::endl;
        }

        // We iterate through each command buffer and record the commands.
        for (size_t i = 0; i < commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            // Possible values for beginInfo.flags can be:
            //      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
            //          Will only be executed once after recording, then will be re-recorded.
            //      VK_COMMAND_BUFFER_USAEG_RENDER_PASS_CONTINUE_BIT
            //          Hints that this is a secondary command buffer that will be entirely
            //          self-contained within a single render pass.
            //      VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
            //          Hints that this command buffer can be re-submitted while it is
            //          already executing.
            beginInfo.flags = 0; // Optional

            // pInheritanceInfo indicates which primary command buffers
            // can call this command buffer (if we are a secondary buffer)
            beginInfo.pInheritanceInfo = nullptr; // Optional

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("Failed to begin recording command buffer");
            }

            // Set up a render pass on the framebuffer[i]
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapchainFramebuffers[i];

            // We render across the entire framebuffer.
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = swapchainExtent;

            // Set the clear values.
            VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;


            // ACTUAL COMMANDS!

            // If we were going to call secondary command buffers inside this command buffer,
            // we would use VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS.
            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            // Bind the graphics pipeline 
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

            vkCmdEndRenderPass(commandBuffers[i]);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to record a command buffer.");
            }

        }
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolCreateInfo{};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        // This will be a command pool for graphics commands.
        poolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        // Possible values:
        //      VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
        //          Hints that command buffers are re-recorded with new commands very often
        //      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
        //          Allows command buffers to be re-recorded individually;
        //          without this, they must all be reset together.
        poolCreateInfo.flags = 0; // Optional

        if (vkCreateCommandPool(logicalDevice, &poolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create a command pool.");
        }
        else {
            std::cout << "Successfully created a command pool." << std::endl;
        }
    }

    void createFramebuffers() {
        swapchainFramebuffers.resize(swapchainImageViews.size()); // One framebuffer per swapchain image.
        for (size_t i = 0; i < swapchainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapchainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferCreateInfo{};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = renderPass;
            framebufferCreateInfo.attachmentCount = 1;
            framebufferCreateInfo.pAttachments = attachments;
            framebufferCreateInfo.width = swapchainExtent.width;
            framebufferCreateInfo.height = swapchainExtent.height;
            framebufferCreateInfo.layers = 1;

            if (vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create one of the framebuffers!");
            }
            else {
                std::cout << "Successfully created framebuffer #" << i << std::endl;
            }

        }
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapchainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // For multisampling.

        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // We don't care about stencils right now.
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // Note: using LAYOUT_UNDEFINED here means potentially discarding
        // the prior contents of the image.
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Image layout before render pass begins.
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Image layout after render pass is done

        // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
        // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : Images to be presented in the swap chain
        // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : Images to be used as destination for a memory copy operation

        // Our basic render has one sub-pass.
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        // Our single color sub-pass.
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Specifically graphics, not compute
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        VkSubpassDependency dependency{};

        // VK_SUBPASS_EXTERNAL refers to the implicit "subpass"
        // that occurs before/after the overall render pass.
        // So, src = EXTERNAL and dst = 0 is a dependency between the
        // implicit subpass before the render pass, and our actual subpass.
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

        // This specifies that our subpass must wait for the COLOR_ATTACHMENT_OUTPUT,
        // i.e. it must wait for the image to be available to render to.
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;

        // Our subpass/operation is in the color attachment stage, and the accessMask
        // is set to the color attachment bit because we will be writing to the color attachment.
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }

    }

    void createGraphicsPipeline() {

        auto vertShaderCode = VulkanUtil::readFile("../src/openwarp/shaders/vert.spv");
        auto fragShaderCode = VulkanUtil::readFile("../src/openwarp/shaders/frag.spv");

        VkShaderModule vertShaderModule = VulkanUtil::createShaderModule(logicalDevice, vertShaderCode);
        VkShaderModule fragShaderModule = VulkanUtil::createShaderModule(logicalDevice, fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // We are using this shader as the vertex shader.

        vertShaderStageInfo.module = vertShaderModule; // Bind the vertex shader module.
        vertShaderStageInfo.pName = "main"; // Standard shader entry point

        // pSpecializationInfo allows you to specify values for shader constants... very useful!
        // It's optional, though.

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // If we were going to add a vertex input stage, it would go here. //

        // For hello-triangle, we are hard-coding vertex data, so
        // we can just leave the vertex input pipeline stage empty.
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

        // Input assembly stage.
        // VkPipelineInputAssemblyStateCreateInfo describes what kind of geometry will be drawn
        // from the vertices (from the vertex shader/input stage), and whether primitive
        // restart should be enabled.

        // Topology can be POINT_LIST, LINE_LIST, LINE_STRIP, TRIANGLE_LIST, TRIANGLE_STRIP, etc

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Create viewport

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapchainExtent.width;
        viewport.height = (float)swapchainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Create scissor rectangle. We'll draw to the entire framebuffer, so the
        // scissor will cover the entire screen.

        VkRect2D scissor{};
        scissor.offset = { 0,0 };
        scissor.extent = swapchainExtent;

        // We need to combine the viewport and scissor into a full viewport state.
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        // Depth clamping will clamp fragments to min/max depth, instead of
        // discarding them.
        rasterizer.depthClampEnable = VK_FALSE; 

        // If rasterizerDiscardEnable is enabled, the rasterizer basically just
        // rejects all incoming geometry.
        rasterizer.rasterizerDiscardEnable = VK_FALSE;

        // Possible alternatives are
        //      VK_POLYGON_MODE_FILL,
        //      VK_POLYGON_MODE_LINE,
        //      VK_POLYGON_MODE_POINT
        // Using any mode other than "fill" requires extra steps.
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

        rasterizer.lineWidth = 1.0f; // Thickness of lines in terms of fragments.

        // Set back culling, and set front face to clockwise winding order.
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

        // Various depth manipulation options.
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        // This specifies multisampling information.
        // TODO enable MSAA
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // If depth/stencil was being used, we would use
        // VkPipelineDepthStencilStateCreateInfo.

        // Color blending, let's use alpha blending.
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        // This structure is an array of all the color blending attachments
        // for all of the framebuffer(s)!
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional


        // Some parts of the graphics pipeline can be marked as dynamic,
        // including the viewport size, line width, and other properties.
        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_LINE_WIDTH
        };

        // This dynamic state struct contains the info on which of the states
        // are to be managed as dynamic properties.
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout.");
        }
        else {
            std::cout << "Successfully created pipeline layout object." << std::endl;
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        // Reference the array of VkPipelineShaderStageCreateInfo structs.
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;

        // Reference all the structures describing the fixed-function stages.
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = nullptr; // Optional

        pipelineInfo.layout = pipelineLayout; // Vulkan handle, not pointer!

        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        // These optional fields are used to construct graphics pipelines from
        // other pipelines, for performance reasons (if the pipelines share
        // several features)
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;


        if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("Error; failed to create graphics pipeline.");
        }
        else {
            std::cout << "Congratulations, the full graphics pipeline has been created." << std::endl;
        }

        vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
        vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);

    }

    void createImageViews() {
        // We assume swapchainImages has already been resized and filled appropriately.
        swapchainImageViews.resize(swapchainImages.size());
        for (size_t i = 0; i < swapchainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Will be a standard 2D framebuffer.
            createInfo.format = swapchainImageFormat; // Same format as the overall swapchain.

            // No swizzles, please.
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0; // No mips, for now.
            createInfo.subresourceRange.levelCount = 1; // Only one level.
            createInfo.subresourceRange.baseArrayLayer = 0; // Indices start at zero.
            createInfo.subresourceRange.layerCount = 1; // One layer only.

            if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image views for swapchain.");
            }
            else {
                std::cout << "Image view for swapchain image " << i << " successfully created." << std::endl;
            }
        }
    }

    void createSwapchain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats); // Image/texture format, etc
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, VK_PRESENT_MODE_FIFO_KHR); // Vsync, etc
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities); // Size/resolution, etc

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
        createInfo.surface = surface; // Attach the surface we should have already acquired through GLFW.
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1; // VR, hmmmmmm.....
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // We assume these are populated because we should have already checked the queue families of this device.
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
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

        if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain.");
        }
        else {
            std::cout << "Successfully created swapchain" << std::endl;
        }

        // Re-query the number of images that actually ended up getting created.
        vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, swapchainImages.data());

        // Save the format and extent of our new swapchain for later.
        swapchainImageFormat = surfaceFormat.format;
        swapchainExtent = extent;

        std::cout << "Acquired handles to created images in swapchain (" << imageCount << " created)" << std::endl;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }
        else {
            // Calculate some reasonable dimensions to use as our swap extent.
            // We need to clamp this so that we don't accidentally exceed the
            // swapchain's maximum or minimum extent capabilities.
            VkExtent2D actualExtent = { WIDTH, HEIGHT };
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            return actualExtent;
        }
    }

    // Chooses an appropriate swap present mode from the list of available presentation modes.
    // An ideal presentation mode can be specified optionally.
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes,
                                            VkPresentModeKHR idealMode = VK_PRESENT_MODE_FIFO_KHR,
                                            VkPresentModeKHR fallbackMode = VK_PRESENT_MODE_FIFO_KHR,
                                            bool verbose = true) {
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

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
                && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat; // We found our desired swapchain format.
            }
        }

        // If we've exhausted all options, the first format is probably our best bet.
        return availableFormats[0];
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {

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

    // Uses GLFW's cross-platform surface creation API to create a window surface
    // for Vulkan to draw to.
    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Error, failed to create window surface.");
        }
        else {
            std::cout << "Created GLFW window surface." << std::endl;
        }
    }

    void createLogicalDevice() {
        std::cout << "Creating/allocating logical device object. " << std::endl;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice, false);

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

        if (enableValidationLayers) { // If we are currently using validation layers at the instance level...
            logicalDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()); // Pass those layers to the device.
            logicalDeviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else { // If we aren't using any instance validation layers, we won't be using any device layers either.
            logicalDeviceCreateInfo.enabledLayerCount = 0;
        }

        // Instantiate the logical device object.
        if (vkCreateDevice(physicalDevice, &logicalDeviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
            throw std::runtime_error("Error: failed to create logical device.");
        }

        // Acquire a handle to the graphics command queue of this device.
        vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
        // Acquire a handle to the presentation command queue of this device.
        vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);

        std::cout << "Logical device object created and queue handles acquired." << std::endl;
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("Couldn't find any GPUs that have Vulkan support :(");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        // If our physical device is STILL the null handle,
        // that means we didn't find any that were suitable.
        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("None of the discovered GPUs are suitable.");
        }
        else {
            VkPhysicalDeviceProperties chosenDeviceProperties;
            vkGetPhysicalDeviceProperties(physicalDevice, &chosenDeviceProperties);
            std::cout << "System is choosing the following device as the physical device for the application:" << std::endl;
            std::cout << "\tName: " << chosenDeviceProperties.deviceName << std::endl;
            std::cout << "\tDriver version: " << chosenDeviceProperties.driverVersion << std::endl;
            std::cout << "\tVendor ID: " << chosenDeviceProperties.vendorID << std::endl;
        }
    }

    // Function for determining whether a particular device is suitable for the application.
    bool isDeviceSuitable(VkPhysicalDevice device) {

        QueueFamilyIndices indices = findQueueFamilies(device, true); // Fetches queue families.

        bool areExtensionsSupported = checkDeviceExtensionSupport(device);

        bool swapchainAdequate = false;
        if (areExtensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            // For the swapchain to be sufficient, we need to have at least one supported format
            // and at least one supported presentation mode.
            swapchainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        // Perhaps in the future, we can rank physical device based on a suitability score.
        return indices.isComplete() && areExtensionsSupported && swapchainAdequate;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        // Create a unique set of extension names from the vector of supported extensions.
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
        std::cout << "Detected " << availableExtensions.size() << " available device extensions" << std::endl;
        for (const auto& ext : availableExtensions) {
            // Erase the supported/available extension from the list of required extensions
            // (if we even require that extension!)
            requiredExtensions.erase(ext.extensionName);
        }

        // If we've "erased" all the required extensions, we're good to go.
        return requiredExtensions.empty();
    }

    // Find all command queue families supported by the indicated device.
    // Also verifies that the device supports a queue family with the
    // "VK_QUEUE_GRAPHICS_BIT" enabled. (Required for display, obviously!)
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, bool verbose = false) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        if (verbose) { std::cout << "Detected " << queueFamilies.size() << " queue families" << std::endl; }
        int i = 0;
        for (const auto& queueFamily : queueFamilies) {

            // Check the device for a graphics queue.
            // (Can't submit graphics commands without it...)
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            // Check the device for support for presenting to surfaces.
            // (Can't display anything without it...)
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }

            // Note, the two above queues may be the same; in which case, they'd have the same index.
            i++; // Increment queue family index.
        }

        if (verbose) {
            if (indices.graphicsFamily.has_value()) {
                std::cout << "\tSuccess, queried device has a graphics queue family (index " << indices.graphicsFamily.value() << ")." << std::endl;
            }
            else {
                std::cout << "\tWARNING: queried device has no queue families with VK_QUEUE_GRAPHICS_BIT enabled." << std::endl;
            }
            if (indices.presentFamily.has_value()) {
                std::cout << "\tSuccess, queried device has a surface presentation queue family (index " << indices.graphicsFamily.value() << ")." << std::endl;
            }
            else {
                std::cout << "\tWARNING: queried device has no queue families that support presenting to surfaces." << std::endl;
            }
        }

        return indices;
    }

    void setupDebugMessenger() {
        if (!enableValidationLayers) return;
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo); // Populates the "createInfo" struct for the debug messenger

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("Failed to setup debug messenger.");
        }

    }

    // Creates/populates the struct for creating a debug messenger.
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void createInstance() {

        std::cout << "Creating Vulkan instance. " << std::endl;

        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("Validation layers were requested that are not available on this system!");
        } else if(enableValidationLayers){
            std::cout << "Validation layers actively running on this application:" << std::endl;
            for(auto const& layer : activeValidationLayers){
                std::cout << "\t" << layer << std::endl;
            }
        }

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
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(activeValidationLayers.size());
            createInfo.ppEnabledLayerNames = activeValidationLayers.data();
            std::cout << "Creating debugMessengerCreateInfo" << std::endl;
            populateDebugMessengerCreateInfo(debugMessengerCreateInfo); // Populate the debug messsenger struct.
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)(&debugMessengerCreateInfo); // Attach our debug messenger createInfo to the instance createInfo.
        }
        else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        auto requiredExtensions = getRequiredExtensions();
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

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance.");
        }
    }

    // Gets the extensions required by GLFW,
    // and adds the VK_EXT_DEBUG_UTILS extension if we are currently debugging.
    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }
        // Wait for any async jobs that were submitted to finish before quitting.
        vkDeviceWaitIdle(logicalDevice);
    }

    void drawFrame() {
        // Wait for the fence before beginning.
        vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        // Determine which image is next in the swapchain.
        vkAcquireNextImageKHR(logicalDevice, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        // Check if a previous frame is using the image we want to render to.
        if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(logicalDevice, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }

        // Mark the current swapchain image as being used by the current frame.
        imagesInFlight[imageIndex] = inFlightFences[currentFrame];

        // Submit the command buffer for rendering.
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // Specify the semaphore to wait on before rendering.
        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        // Which stage should we sit in while waiting for the semaphore?
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);
        // Submit the command buffer to the graphics queue.
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores; // We have to wait on the renderFinished semaphore.

        VkSwapchainKHR swapchainsToPresent[] = { swapchain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchainsToPresent;
        presentInfo.pImageIndices = &imageIndex; // Which image?
        presentInfo.pResults = nullptr; // Can use to check the success of the presentation

        vkQueuePresentKHR(presentQueue, &presentInfo);

        lastFrameTime = glfwGetTime();
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void cleanup() {
        std::cout << "Cleaning up" << std::endl;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
        }
        vkDestroyCommandPool(logicalDevice, commandPool, nullptr); // Will automatically clean up command buffers, too.
        for (auto framebuffer : swapchainFramebuffers) {
            vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
        }
        vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
        vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
        // We created these imageViews ourself, so we need to clean them up.
        for (auto imageView : swapchainImageViews) {
            vkDestroyImageView(logicalDevice, imageView, nullptr);
        }
        vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
        vkDestroyDevice(logicalDevice, nullptr); // Destroy our logical device first.
        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
        vkDestroySurfaceKHR(instance, surface, nullptr); // Surface must be destroyed before the instance!
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        std::cout << "Available validation layers (" << layerCount << ") on this system: " << std::endl;
        for (const auto& layer : availableLayers) { std::cout << "\t" << layer.layerName << std::endl; }


        // See if all of our "preferred" validation layers are available.
        bool preferredAvailable = checkIfAllLayersSupported(validationLayers, availableLayers);

        
        if(!preferredAvailable){
            // If our preferred layers are not available, check the fallback layers.
            bool fallbackAvailable = checkIfAllLayersSupported(fallbackValidationLayers, availableLayers);

            // If even the fallback layers are not available, we fail the check entirely.
            if(!fallbackAvailable){
                return false;
            } else {
                // If the fallback is available, we copy 
                // the fallback layers vector to the "active validation layers"
                // vector for future bookkeeping.
                std::cout << "WARNING: Using fallback validation layer! This is probably because you have " <<
                                "an old Vulkan SDK without the newest validation layers." << std::endl;
                activeValidationLayers = fallbackValidationLayers;
                return true;
            }
        } else {

            // Our preferred layers are available. Copy the preferred layers
            // to the active layers vector and return true.
            activeValidationLayers = validationLayers;
            return true;
        }
    }

    // Takes a vector of layer names, and vector of VkLayerProperties and verifies that all queried layers are present in the
    // vector of available layers.
    bool checkIfAllLayersSupported(const std::vector<const char*> queriedLayers, const std::vector<VkLayerProperties> availableLayers){
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

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            // Message is important enough to show
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        }
        

        return VK_FALSE;
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception & e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}