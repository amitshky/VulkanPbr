#pragma once

#include <vulkan/vulkan.h>
#include "engine/types.h"

namespace inits {

// debug messenger
VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo(
	PFN_vkDebugUtilsMessengerCallbackEXT pfnDebugCallback);

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator);


// resource pools
VkCommandPoolCreateInfo CommandPoolCreateInfo(const QueueFamilyIndices& queueIndices);

VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo(VkDescriptorPoolSize* poolSizes,
	const uint32_t poolSizeCount,
	uint32_t maxSets);


// render pass
VkAttachmentDescription AttachmentDescription(VkFormat format,
	VkSampleCountFlagBits samples,
	VkImageLayout initialLayout,
	VkImageLayout finalLayout);

VkAttachmentReference AttachmentReference(uint32_t attachmentIndex, VkImageLayout layout);

VkSubpassDescription SubpassDescription(uint32_t colorAttachmentCount,
	const VkAttachmentReference* pColorAttachments,
	const VkAttachmentReference* pDepthStencilAttachment,
	const VkAttachmentReference* pResolveAttachments);

VkSubpassDependency SubpassDependency(uint32_t srcSubpass,
	uint32_t dstSubpass,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask,
	VkAccessFlags srcAccessMask,
	VkAccessFlags dstAccessMask);

VkRenderPassCreateInfo RenderPassCreateInfo(uint32_t attachmentCount,
	const VkAttachmentDescription* pAttachments,
	uint32_t subpassCount,
	const VkSubpassDescription* pSubpasses,
	uint32_t dependencyCount,
	const VkSubpassDependency* pDependencies);


// descriptors
VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(uint32_t binding,
	VkDescriptorType descriptorType,
	uint32_t descriptorCount,
	VkShaderStageFlags stageFlags);

VkDescriptorBufferInfo DescriptorBufferInfo(VkBuffer buffer,
	VkDeviceSize offset,
	VkDeviceSize range);

VkDescriptorImageInfo DescriptorImageInfo(VkSampler sampler, VkImageView imageView);

VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorSet dstSet,
	uint32_t dstBinding,
	VkDescriptorType descriptorType,
	uint32_t descriptorCount,
	const VkDescriptorBufferInfo* pBufferInfo,
	const VkDescriptorImageInfo* pImageInfo);


// pipeline
VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo(
	uint32_t vertexBindingDescriptionCount,
	const VkVertexInputBindingDescription* pVertexBindingDescriptions,
	uint32_t vertexAttributeDescriptionCount,
	const VkVertexInputAttributeDescription* pVertexAttributeDescriptions);

VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo(
	VkPrimitiveTopology topology);

VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo(uint32_t viewportCount,
	uint32_t scissorCount);

VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo(
	VkCullModeFlags cullMode,
	VkFrontFace frontFace);

VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo(
	VkBool32 sampleShadingEnable,
	VkSampleCountFlagBits rasterizationSamples,
	float minSampleShading);

VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo(VkBool32 depthTestEnable,
	VkBool32 depthWriteEnable,
	VkCompareOp depthCompareOp);

VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo(
	const VkPipelineColorBlendAttachmentState& colorBlendAttachment);

VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo(uint32_t dynamicStateCount,
	const VkDynamicState* pDynamicStates);


} // namespace inits