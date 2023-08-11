#pragma once

#include <vulkan/vulkan.h>


class VulkanContext
{
public:
	VulkanContext(const char* title);
	~VulkanContext();
	VulkanContext(const VulkanContext&) = delete;
	VulkanContext(VulkanContext&&) = delete;
	VulkanContext& operator=(const VulkanContext&) = delete;
	VulkanContext& operator=(VulkanContext&&) = delete;

	[[nodiscard]] inline VkInstance GetInstance() const { return m_VulkanInstance; }

private:
	void Init(const char* title);
	void Cleanup();

	VkInstance m_VulkanInstance{};
	VkDebugUtilsMessengerCreateInfoEXT m_DebugMessengerInfo{};
	VkDebugUtilsMessengerEXT m_DebugMessenger{};
};