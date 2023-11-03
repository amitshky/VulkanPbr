#pragma once

#include <vulkan/vulkan.h>
#include "imgui.h"


class ImGuiOverlay
{
public:
	static void Init(VkInstance vulkanInstance,
		VkPhysicalDevice physicalDevice,
		VkDevice deviceVk,
		uint32_t graphicsQueueIndex,
		VkQueue graphicsQueue,
		VkSampleCountFlagBits msaaSampleCount,
		VkRenderPass renderPass,
		VkCommandPool commandPool,
		uint32_t imageCount);
	static void Cleanup(VkDevice deviceVk);
	static void Begin();
	static void End(VkCommandBuffer commandBuffer);

private:
	static void CheckVkResult(VkResult err);

	static VkDescriptorPool s_DescriptorPool;
};