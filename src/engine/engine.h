#pragma once

#include <cstdint>
#include <memory>
#include <chrono>
#include <vulkan/vulkan.h>
#include "core/window.h"
#include "engine/types.h"
#include "engine/vulkanContext.h"
#include "engine/device.h"
#include "engine/camera.h"
#include "engine/model.h"

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

	static void CreateVertexBuffer(const std::vector<Vertex>& vertices,
		VkBuffer& vertexBuffer,
		VkDeviceMemory& vertexBufferMemory);
	static void CreateIndexBuffer(const std::vector<uint32_t>& indices,
		VkBuffer& indexBuffer,
		VkDeviceMemory& indexBufferMemory);

private:
	explicit Engine(const char* title, const uint64_t width = 1280, const uint64_t height = 720);

	void Init(const char* title, const uint64_t width, const uint64_t height);
	void Cleanup();
	void Draw(float deltatime);
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

	void CreateTextureSampler();
	void CreateTextureImage(const char* texturePath,
		VkImage& textureImage,
		VkDeviceMemory& textureImageMem,
		VkFormat format,
		uint32_t& miplevels);

	void CreateCubemap(const std::array<const char*, 6>& cubemapPaths,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		uint32_t& miplevels,
		VkImage& cubemapImage,
		VkDeviceMemory& cubemapImageMem,
		VkImageView& cubemapImageView);
	void CreateCubemapDescriptorSetLayout();
	void CreateCubemapDescriptorSets();
	void CreateCubemapPipelineLayout();
	void CreateCubemapPipeline(const char* vertShaderPath, const char* fragShaderPath);
	void CreateCubemapVertexBuffer();

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

	std::vector<VkBuffer> m_SceneUniformBuffers;
	std::vector<VkDeviceMemory> m_SceneUniformBufferMemory;
	std::vector<VkBuffer> m_MatUniformBuffers;
	std::vector<VkDeviceMemory> m_MatUniformBufferMemory;
	std::vector<VkBuffer> m_LightUniformBuffers;
	std::vector<VkDeviceMemory> m_LightUniformBufferMemory;

	VkSampler m_TextureImageSampler{};
	std::vector<VkImage> m_TextureImages;
	std::vector<VkImageView> m_TextureImageViews;
	std::vector<VkDeviceMemory> m_TextureImageMems;

	VkPipeline m_Pipeline{};
	std::vector<VkCommandBuffer> m_CommandBuffers;

	std::unique_ptr<Model> m_Model;

	// cubemap
	std::vector<Vertex> m_CubemapVertices;
	VkImage m_CubemapImage{};
	VkImageView m_CubemapImageView{};
	VkDeviceMemory m_CubemapImageMem{};
	std::vector<VkBuffer> m_CubemapUniformBuffers{};
	std::vector<VkDeviceMemory> m_CubemapUniformBufferMem{};
	VkDescriptorSetLayout m_CubemapDescriptorSetLayout{};
	std::vector<VkDescriptorSet> m_CubemapDescriptorSets;
	VkPipelineLayout m_CubemapPipelineLayout{};
	VkPipeline m_CubemapPipeline{};
	VkBuffer m_CubemapVertexBuffer{};
	VkDeviceMemory m_CubemapVertexBufferMem{};

	// synchronization objects
	// used to acquire swapchain images
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	// signaled when command buffers have finished execution
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;

	std::unique_ptr<Camera> m_Camera;

	VkCommandBuffer m_ActiveCommandBuffer{};
	uint32_t m_CurrentFrameIndex = 0;
	uint32_t m_NextFrameIndex = 0; // acquired from swapchain
	bool m_FramebufferResized = false;
	float m_AspectRatio = 0.0f;

	uint32_t m_LastFps = 0;
	uint32_t m_FrameCounter = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_LastFrameTime;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_FpsTimePoint;
};