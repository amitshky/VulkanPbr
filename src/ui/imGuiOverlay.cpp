#include "ui/imGuiOverlay.h"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "core/core.h"
#include "engine/engine.h"
#include "engine/initializers.h"
#include "utils/utils.h"


VkDescriptorPool ImGuiOverlay::s_DescriptorPool = nullptr;

void ImGuiOverlay::Init(VkInstance vulkanInstance,
	VkPhysicalDevice physicalDevice,
	VkDevice deviceVk,
	uint32_t graphicsQueueIndex,
	VkQueue graphicsQueue,
	VkSampleCountFlagBits msaaSampleCount,
	VkRenderPass renderPass,
	VkCommandPool commandPool,
	uint32_t imageCount)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	VkDescriptorPoolSize poolSizes[] = {
		{VK_DESCRIPTOR_TYPE_SAMPLER,                 100},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100},
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          100},
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          100},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   100},
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   100},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         100},
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         100},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100},
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100},
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       100},
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo =
		inits::DescriptorPoolCreateInfo(poolSizes, std::size(poolSizes), 100);
	ErrCheck(vkCreateDescriptorPool(deviceVk, &descriptorPoolInfo, nullptr, &s_DescriptorPool) != VK_SUCCESS,
		"Failed to create descriptor pool!");

	ImGui_ImplGlfw_InitForVulkan(Engine::GetWindowHandle(), true);
	ImGui_ImplVulkan_InitInfo info{};
	info.Instance = vulkanInstance;
	info.PhysicalDevice = physicalDevice;
	info.Device = deviceVk;
	info.QueueFamily = graphicsQueueIndex;
	info.Queue = graphicsQueue;
	info.DescriptorPool = s_DescriptorPool;
	info.PipelineCache = VK_NULL_HANDLE;
	info.Subpass = 0;
	info.MinImageCount = imageCount;
	info.ImageCount = imageCount;
	info.MSAASamples = msaaSampleCount;
	info.Allocator = nullptr;
	info.CheckVkResultFn = CheckVkResult;
	ImGui_ImplVulkan_Init(&info, renderPass);

	VkCommandBuffer cmdBuff = utils::BeginSingleTimeCommands(deviceVk, commandPool);
	ImGui_ImplVulkan_CreateFontsTexture(cmdBuff);
	utils::EndSingleTimeCommands(cmdBuff, deviceVk, commandPool, graphicsQueue);

	vkDeviceWaitIdle(deviceVk);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void ImGuiOverlay::Cleanup(VkDevice deviceVk)
{
	vkDestroyDescriptorPool(deviceVk, s_DescriptorPool, nullptr);
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void ImGuiOverlay::Begin()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGuiOverlay::End(VkCommandBuffer commandBuffer)
{
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void ImGuiOverlay::CheckVkResult(VkResult err)
{
	if (err == VK_SUCCESS)
		return;

	Logger::Error("ImGui::Vulkan: {}\n", static_cast<uint32_t>(err));
}