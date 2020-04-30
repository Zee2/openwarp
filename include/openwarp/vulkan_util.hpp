#pragma once


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <fstream>

class VulkanUtil {
public:
	static std::vector<char> readFile(const std::string& filename);
	static VkShaderModule createShaderModule(const VkDevice device, const std::vector<char>& code);
};
