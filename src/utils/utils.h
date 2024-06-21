#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <vulkan/vulkan.h>
#include "engine/types.h"
#include "engine/device.h"

namespace utils {


// `VKAPI_ATTR` and `VKAPI_CALL` ensures the right signature for Vulkan
VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbakck,
	void* pUserData);

std::vector<const char*> GetRequiredExtensions();
bool CheckValidationLayerSupport();


// device details functions
bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
uint32_t FindMemoryType(VkPhysicalDeviceMemoryProperties deviceMemProperties,
	uint32_t typeFilter,
	VkMemoryPropertyFlags properties);
VkFormat FindSupportedFormat(const VkPhysicalDevice physicalDevice,
	const std::vector<VkFormat>& canditateFormats,
	VkImageTiling tiling,
	VkFormatFeatureFlags features);


// swapchain
VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities,
	std::function<void(int* width, int* height)> pfnGetFramebufferSize);
VkFormat FindDepthFormat(const VkPhysicalDevice physicalDevice);


// images and buffers
void CreateImage(const std::unique_ptr<Device>& device,
	uint32_t width,
	uint32_t height,
	uint32_t miplevels,
	uint32_t arrayLayers,
	VkSampleCountFlagBits numSamples,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkImageCreateFlags imageCreateFlags,
	VkMemoryPropertyFlags properties,
	VkImage& image,
	VkDeviceMemory& imageMemory);

VkImageView CreateImageView(VkDevice deviceVk,
	VkImage image,
	VkFormat format,
	VkImageViewType imageViewType,
	VkImageAspectFlags aspectFlags,
	uint32_t miplevels,
	uint32_t layerCount);

void CreateBuffer(const std::unique_ptr<Device>& device,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& bufferMemory);

void CopyBuffer(const std::unique_ptr<Device>& device,
	VkCommandPool commandPool,
	VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	VkDeviceSize size);

void CopyBufferToImage(const std::unique_ptr<Device>& device,
	VkCommandPool commandPool,
	VkBuffer buffer,
	VkImage image,
	uint32_t width,
	uint32_t height,
	uint32_t layerCount);

void GenerateMipmaps(const std::unique_ptr<Device>& device,
	VkCommandPool commandPool,
	VkImage image,
	VkFormat format,
	int32_t width,
	int32_t height,
	uint32_t mipLevels);

void TransitionImageLayout(const std::unique_ptr<Device>& device,
	VkCommandPool commandPool,
	VkImage image,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t miplevels,
	uint32_t layerCount);


// commands
VkCommandBuffer BeginSingleTimeCommands(VkDevice deviceVk, VkCommandPool commandPool);
void EndSingleTimeCommands(VkCommandBuffer cmdBuff,
	VkDevice deviceVk,
	VkCommandPool commandPool,
	VkQueue graphicsQueue);


void CalcTangentVectors(std::vector<Vertex>& vertices);
std::pair<std::vector<Vertex>, std::vector<uint32_t>> GenerateVerticesAndIndices(
	const std::vector<Vertex>& vertices);
std::pair<std::vector<Vertex>, std::vector<uint32_t>> GenerateCubeData();
std::vector<Vertex> GenerateSkyboxData();

} // namespace utils