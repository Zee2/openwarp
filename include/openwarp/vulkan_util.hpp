#pragma once


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <optional>
#include <fstream>

#include "openwarp.hpp"



class Openwarp::OpenwarpUtils {
public:
	static std::vector<char> readFile(const std::string& filename);
	static VkShaderModule createShaderModule(const VkDevice device, const std::vector<char>& code);
	static std::vector<const char*> getGlfwRequiredExtensions();
	

	

	// Wrapper function that loads the function address of vkCreateDebugUtilsMessengerEXT for us.
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
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

	// Another wrapper/proxy function that loads the vkDestroyDebugUtilsMessengerEXT
	// (the destructor to the object that is created in the above proxy function)
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
										VkDebugUtilsMessengerEXT debugMessenger,
										const VkAllocationCallbacks* pAllocator) {

		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
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

	// Find all command queue families supported by the indicated device.
    // Also verifies that the device supports a queue family with the
    // "VK_QUEUE_GRAPHICS_BIT" enabled. (Required for display, obviously!)
    static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, bool verbose = false) {
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
};


