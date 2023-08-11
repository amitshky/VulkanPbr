#pragma once

#include <vulkan/vulkan.h>
#include "engine/types.h"


class Device
{
public:
	Device(VkInstance vulkanInstance, VkSurfaceKHR windowSurface);
	~Device();
	Device(const Device&) = delete;
	Device(Device&&) = delete;
	Device& operator=(const Device&) = delete;
	Device& operator=(Device&&) = delete;

	[[nodiscard]] inline VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
	[[nodiscard]] inline VkDevice GetDevice() const { return m_VulkanDevice; }

	[[nodiscard]] inline QueueFamilyIndices GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
	[[nodiscard]] inline VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
	[[nodiscard]] inline VkQueue GetPresentQueue() const { return m_PresentQueue; }

	[[nodiscard]] inline VkPhysicalDeviceFeatures GetDeviceFeatures() const { return m_PhysicalDeviceFeatures; }
	[[nodiscard]] inline VkPhysicalDeviceProperties GetDeviceProperties() const { return m_PhysicalDeviceProperties; }
	[[nodiscard]] inline VkPhysicalDeviceMemoryProperties GetDeviceMemoryProperties() const
	{
		return m_PhysicalDeviceMemoryProperties;
	}

	[[nodiscard]] inline VkSampleCountFlagBits GetMsaaSamples() const { return m_MsaaSamples; }

	static SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface);

private:
	void Init(VkInstance vulkanInstance, VkSurfaceKHR windowSurface);
	void Cleanup();

	static bool IsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface);
	static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface);
	static VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDeviceProperties properties);

	void PickPhysicalDevice(VkInstance vulkanInstance, VkSurfaceKHR windowSurface);
	void CreateLogicalDevice(VkSurfaceKHR windowSurface);


	VkPhysicalDevice m_PhysicalDevice{};
	VkDevice m_VulkanDevice{};

	VkPhysicalDeviceFeatures m_PhysicalDeviceFeatures{};
	VkPhysicalDeviceProperties m_PhysicalDeviceProperties{};
	VkPhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties{};

	QueueFamilyIndices m_QueueFamilyIndices{};
	VkQueue m_PresentQueue{};
	VkQueue m_GraphicsQueue{};

	VkSampleCountFlagBits m_MsaaSamples{};
};