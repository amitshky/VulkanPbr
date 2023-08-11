#pragma once

#include <cstdint>
#include <memory>
#include <chrono>
#include <vulkan/vulkan.h>
#include "core/window.h"
#include "engine/types.h"
#include "engine/vulkanContext.h"
#include "engine/device.h"

class Engine
{
public:
	~Engine();
	Engine(const Engine&) = delete;
	Engine(Engine&&) = delete;
	Engine& operator=(const Engine&) = delete;
	Engine& operator=(Engine&&) = delete;

	[[nodiscard]] static Engine* Create(const char* title, const uint64_t width = 1280, const uint64_t height = 720);
	[[nodiscard]] static inline Engine* GetInstance() { return s_Instance; }
	[[nodiscard]] static inline GLFWwindow* GetWindowHandle() { return s_Instance->m_Window->GetWindowHandle(); }

	void Run();

private:
	explicit Engine(const char* title, const uint64_t width = 1280, const uint64_t height = 720);

	void Init(const char* title, const uint64_t width, const uint64_t height);
	void Cleanup();
	void Draw();
	void BeginScene();
	void EndScene();
	void OnUiRender();
	float CalcFps();


	void CreateCommandPool();
	void CreateDescriptorPool();

	void CreateSwapchain();
	void CreateSwapchainImageViews();
	void RecreateSwapchain();
	void CleanupSwapchain();

	void CreateRenderPass();
	void CreateColorResource();
	void CreateDepthResource();
	void CreateFramebuffers();

	void CreateUniformBuffers();
	void UpdateUniformBuffers();

	void CreateDescriptorSetLayout();
	void CreateDescriptorSets();
	void CreatePipelineLayout();

	void CreatePipeline(const char* vertShaderPath, const char* fragShaderPath);
	void CreateCommandBuffers();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateSyncObjects();

	// event callbacks
	void ProcessInput();
	void OnCloseEvent();
	void OnResizeEvent(int width, int height);
	void OnMouseMoveEvent(double xpos, double ypos);
	void OnKeyEvent(int key, int scancode, int action, int mods);


	bool m_IsRunning = true;
	static Engine* s_Instance;

	std::unique_ptr<Window> m_Window;
	std::unique_ptr<VulkanContext> m_VulkanContext;
	std::unique_ptr<Device> m_Device;

	VkCommandPool m_CommandPool{};
	VkDescriptorPool m_DescriptorPool{};

	VkSwapchainKHR m_Swapchain{};
	std::vector<VkImage> m_SwapchainImages;
	VkFormat m_SwapchainImageFormat{};
	VkExtent2D m_SwapchainExtent{};
	std::vector<VkImageView> m_SwapchainImageViews;

	VkRenderPass m_RenderPass{};

	VkImage m_ColorImage{};
	VkDeviceMemory m_ColorImageMemory{};
	VkImageView m_ColorImageView{};
	VkImage m_DepthImage{};
	VkDeviceMemory m_DepthImageMemory{};
	VkImageView m_DepthImageView{};
	std::vector<VkFramebuffer> m_SwapchainFramebuffers;

	VkDescriptorSetLayout m_DescriptorSetLayout{};
	VkPipelineLayout m_PipelineLayout{};
	std::vector<VkDescriptorSet> m_DescriptorSets;
	std::vector<VkBuffer> m_UniformBuffers;
	std::vector<VkDeviceMemory> m_UniformBufferMemory;

	VkPipeline m_Pipeline{};

	std::vector<VkCommandBuffer> m_CommandBuffers;

	std::vector<Vertex> m_Vertices;
	std::vector<uint32_t> m_Indices;
	VkBuffer m_VertexBuffer{};
	VkDeviceMemory m_VertexBufferMemory{};
	VkBuffer m_IndexBuffer{};
	VkDeviceMemory m_IndexBufferMemory{};

	// synchronization objects
	// used to acquire swapchain images
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	// signaled when command buffers have finished execution
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;

	VkCommandBuffer m_ActiveCommandBuffer{};
	uint32_t m_CurrentFrameIndex = 0;
	uint32_t m_NextFrameIndex = 0; // acquired from swapchain
	bool m_FramebufferResized = false;

	uint32_t m_LastFps = 0;
	uint32_t m_FrameCounter = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_LastFrameTime;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_FpsTimePoint;
};