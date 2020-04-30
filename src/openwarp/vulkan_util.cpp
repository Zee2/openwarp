

#include <vulkan_util.hpp>

std::vector<char> VulkanUtil::readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to read file.");
	}

	// std::ios::ate starts us at the end of the ifstream.
	// Therefore, our position in the ifstream tells us the filesize.
	size_t filesize = (size_t)file.tellg();
	std::vector<char> buffer(filesize);
	file.seekg(0); // Seek back to the beginning of the file.
	file.read(buffer.data(), filesize);
	file.close();

	return buffer;
}

VkShaderModule VulkanUtil::createShaderModule(const VkDevice device, const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module.");
	}

	return shaderModule;
}
