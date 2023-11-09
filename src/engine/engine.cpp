#include "engine/engine.h"

#include "stb_image.h"
#include "glm/gtc/matrix_inverse.hpp"
#include "core/core.h"
#include "core/input.h"
#include "engine/initializers.h"
#include "engine/shader.h"
#include "ui/imGuiOverlay.h"
#include "utils/utils.h"

Engine* Engine::s_Instance = nullptr;

Engine::Engine(const char* title, const uint64_t width, const uint64_t height)
{
	s_Instance = this;
	Init(title, width, height);
}

Engine::~Engine()
{
	Cleanup();
}

Engine* Engine::Create(const char* title, const uint64_t width, const uint64_t height)
{
	if (s_Instance == nullptr)
		return new Engine(title, width, height);

	return s_Instance;
}

void Engine::Init(const char* title, const uint64_t width, const uint64_t height)
{
	m_Window = std::make_unique<Window>(WindowProps{ title, width, height });
	// set window event callbacks
	m_Window->SetCloseEventCallbackFn(BIND_FN(Engine::OnCloseEvent));
	m_Window->SetResizeEventCallbackFn(BIND_FN(Engine::OnResizeEvent));
	m_Window->SetMouseEventCallbackFn(BIND_FN(Engine::OnMouseMoveEvent));
	m_Window->SetKeyEventCallbackFn(BIND_FN(Engine::OnKeyEvent));

	m_VulkanContext = std::make_unique<VulkanContext>(title);
	m_Window->CreateWindowSurface(m_VulkanContext->GetInstance());
	m_Device = std::make_unique<Device>(m_VulkanContext->GetInstance(), m_Window->GetWindowSurface());

	Logger::Info("{} application initialized!", title);
	CreateCommandPool();
	CreateDescriptorPool();

	CreateSwapchain();
	CreateSwapchainImageViews();
	CreateRenderPass();
	CreateColorResource();
	CreateDepthResource();
	CreateFramebuffers();

	CreateCommandBuffers();
	CreateUniformBuffers();

	bool pbr = true;
	m_Model = std::make_unique<Model>("assets/models/backpack/backpack.obj", pbr, true);

	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	uint32_t miplevels = 0; // miplevels have been generated but not used for now

	CreateTextureSampler();

	std::vector<std::string> texturePaths = m_Model->GetTexturePaths();
	m_TextureImages.resize(texturePaths.size());
	m_TextureImageViews.resize(texturePaths.size());
	m_TextureImageMems.resize(texturePaths.size());

	for (int32_t i = 0; i < texturePaths.size(); ++i)
	{
		CreateTextureImage(texturePaths[i].c_str(), m_TextureImages[i], m_TextureImageMems[i], format, miplevels);
		m_TextureImageViews[i] = utils::CreateImageView(
			m_Device->GetDevice(), m_TextureImages[i], format, VK_IMAGE_VIEW_TYPE_2D, aspectFlags, miplevels, 1);
	}

	CreateDescriptorSetLayout();
	CreateDescriptorSets();
	CreatePipelineLayout();

	if (pbr)
		CreatePipeline("assets/shaders/out/normalMapInvTBN.vert.spv", "assets/shaders/out/normalMapInvTBN.frag.spv");
	else
		CreatePipeline("assets/shaders/out/phongLighting.vert.spv", "assets/shaders/out/phongLighting.frag.spv");

	// skybox
	std::array<const char*, 6> cubemapPaths{
		"assets/textures/skybox/right.jpg",
		"assets/textures/skybox/left.jpg",
		"assets/textures/skybox/top.jpg",
		"assets/textures/skybox/bottom.jpg",
		"assets/textures/skybox/front.jpg",
		"assets/textures/skybox/back.jpg",
	};
	CreateCubemap(cubemapPaths, format, aspectFlags, miplevels, m_CubemapImage, m_CubemapImageMem, m_CubemapImageView);
	CreateCubemapDescriptorSetLayout();
	CreateCubemapDescriptorSets();
	CreateCubemapPipelineLayout();
	CreateCubemapPipeline("assets/shaders/out/skybox.vert.spv", "assets/shaders/out/skybox.frag.spv");
	CreateCubemapVertexBuffer();

	CreateSyncObjects();

	m_Camera = std::make_unique<Camera>(m_AspectRatio);

	ImGuiOverlay::Init(m_VulkanContext->GetInstance(),
		m_Device->GetPhysicalDevice(),
		m_Device->GetDevice(),
		m_Device->GetQueueFamilyIndices().graphicsFamily.value(),
		m_Device->GetGraphicsQueue(),
		m_Device->GetMsaaSamples(),
		m_RenderPass,
		m_CommandPool,
		Config::maxFramesInFlight);
}

void Engine::Cleanup()
{
	vkDeviceWaitIdle(m_Device->GetDevice());

	ImGuiOverlay::Cleanup(m_Device->GetDevice());

	for (size_t i = 0; i < Config::maxFramesInFlight; ++i)
	{
		vkDestroySemaphore(m_Device->GetDevice(), m_ImageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_Device->GetDevice(), m_RenderFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_Device->GetDevice(), m_InFlightFences[i], nullptr);
	}

	// skybox
	vkDestroyImage(m_Device->GetDevice(), m_CubemapImage, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_CubemapImageMem, nullptr);
	vkDestroyImageView(m_Device->GetDevice(), m_CubemapImageView, nullptr);
	vkDestroyPipelineLayout(m_Device->GetDevice(), m_CubemapPipelineLayout, nullptr);
	vkDestroyPipeline(m_Device->GetDevice(), m_CubemapPipeline, nullptr);
	vkDestroyDescriptorSetLayout(m_Device->GetDevice(), m_CubemapDescriptorSetLayout, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_CubemapVertexBufferMem, nullptr);
	vkDestroyBuffer(m_Device->GetDevice(), m_CubemapVertexBuffer, nullptr);

	vkDestroySampler(m_Device->GetDevice(), m_TextureImageSampler, nullptr);

	for (int32_t i = 0; i < m_TextureImages.size(); ++i)
	{
		vkFreeMemory(m_Device->GetDevice(), m_TextureImageMems[i], nullptr);
		vkDestroyImageView(m_Device->GetDevice(), m_TextureImageViews[i], nullptr);
		vkDestroyImage(m_Device->GetDevice(), m_TextureImages[i], nullptr);
	}

	m_Model->Cleanup(m_Device->GetDevice());

	vkDestroyPipeline(m_Device->GetDevice(), m_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_Device->GetDevice(), m_PipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_Device->GetDevice(), m_DescriptorSetLayout, nullptr);

	for (uint64_t i = 0; i < Config::maxFramesInFlight; ++i)
	{
		vkFreeMemory(m_Device->GetDevice(), m_CubemapUniformBufferMem[i], nullptr);
		vkDestroyBuffer(m_Device->GetDevice(), m_CubemapUniformBuffers[i], nullptr);

		vkFreeMemory(m_Device->GetDevice(), m_MatUniformBufferMemory[i], nullptr);
		vkDestroyBuffer(m_Device->GetDevice(), m_MatUniformBuffers[i], nullptr);

		vkFreeMemory(m_Device->GetDevice(), m_SceneUniformBufferMemory[i], nullptr);
		vkDestroyBuffer(m_Device->GetDevice(), m_SceneUniformBuffers[i], nullptr);
	}

	CleanupSwapchain();
	vkDestroyRenderPass(m_Device->GetDevice(), m_RenderPass, nullptr);

	vkDestroyDescriptorPool(m_Device->GetDevice(), m_DescriptorPool, nullptr);
	vkDestroyCommandPool(m_Device->GetDevice(), m_CommandPool, nullptr);

	m_Window->DestroyWindowSurface(m_VulkanContext->GetInstance());
}

void Engine::Run()
{
	m_LastFrameTime = std::chrono::high_resolution_clock::now();
	while (m_IsRunning)
	{
		const float deltatime = CalcFps();
		Draw(deltatime);
		ProcessInput();
		m_Window->OnUpdate();
	}
}

void Engine::Draw(float deltatime)
{
	BeginScene();

	uint32_t dynamicOffset = 0;
	VkDeviceSize offset = 0;

	// model
	vkCmdBindPipeline(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	vkCmdBindDescriptorSets(m_ActiveCommandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_PipelineLayout,
		0,
		1,
		&m_DescriptorSets[m_CurrentFrameIndex],
		1,
		&dynamicOffset);

	m_Model->Draw(m_ActiveCommandBuffer);

	// skybox // draw skybox at the last
	vkCmdBindPipeline(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CubemapPipeline);
	vkCmdBindDescriptorSets(m_ActiveCommandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_CubemapPipelineLayout,
		0,
		1,
		&m_CubemapDescriptorSets[m_CurrentFrameIndex],
		1,
		&dynamicOffset);
	vkCmdBindVertexBuffers(m_ActiveCommandBuffer, 0, 1, &m_CubemapVertexBuffer, &offset);
	vkCmdDraw(m_ActiveCommandBuffer, static_cast<uint32_t>(m_CubemapVertices.size()), 1, 0, 0);

	UpdateUniformBuffers();
	m_Camera->OnUpdate(deltatime);
	OnUiRender();

	EndScene();
}

void Engine::UpdateUniformBuffers()
{
	SceneUBO scene{};
	scene.cameraPos = m_Camera->GetCameraPosition();
	scene.lightPos[0] = glm::vec4(0.0f, 0.0f, 30.0f, 0.0f);
	scene.lightPos[1] = glm::vec4(0.0f, 30.0f, 0.0f, 0.0f);
	scene.lightPos[2] = glm::vec4(30.0f, 0.0f, 0.0f, 0.0f);
	scene.lightPos[3] = glm::vec4(-30.0f, 0.0f, 0.0f, 0.0f);
	scene.lightColors = glm::vec3(500.0f);

	void* data = nullptr;
	vkMapMemory(
		m_Device->GetDevice(), m_SceneUniformBufferMemory[m_CurrentFrameIndex], 0, SceneUBO::GetSize(), 0, &data);
	memcpy(data, &scene, SceneUBO::GetSize());
	vkUnmapMemory(m_Device->GetDevice(), m_SceneUniformBufferMemory[m_CurrentFrameIndex]);

	MatrixUBO mat{};
	mat.model = glm::mat4(1.0);
	mat.model = glm::translate(mat.model, glm::vec3(0.0f, 0.0f, 0.0f));
	mat.model = glm::scale(mat.model, glm::vec3(0.5f));
	mat.viewProj = m_Camera->GetViewProjectionMatrix();
	mat.normal = glm::inverseTranspose(mat.model);

	vkMapMemory(
		m_Device->GetDevice(), m_MatUniformBufferMemory[m_CurrentFrameIndex], 0, MatrixUBO::GetSize(), 0, &data);
	memcpy(data, &mat, MatrixUBO::GetSize());
	vkUnmapMemory(m_Device->GetDevice(), m_MatUniformBufferMemory[m_CurrentFrameIndex]);

	// skybox
	mat.viewProj =
		m_Camera->GetProjectionMatrix()
		* glm::mat4(glm::mat3(m_Camera->GetViewMatrix())); // remove the translation component from the view matrix
	vkMapMemory(
		m_Device->GetDevice(), m_CubemapUniformBufferMem[m_CurrentFrameIndex], 0, MatrixUBO::GetSize(), 0, &data);
	memcpy(data, &mat, MatrixUBO::GetSize());
	vkUnmapMemory(m_Device->GetDevice(), m_CubemapUniformBufferMem[m_CurrentFrameIndex]);
}

void Engine::BeginScene()
{
	// wait for previous frame to signal the fence
	vkWaitForFences(m_Device->GetDevice(), 1, &m_InFlightFences[m_CurrentFrameIndex], VK_TRUE, UINT64_MAX);

	const VkResult result = vkAcquireNextImageKHR(m_Device->GetDevice(),
		m_Swapchain,
		UINT64_MAX,
		m_ImageAvailableSemaphores[m_CurrentFrameIndex],
		VK_NULL_HANDLE,
		&m_NextFrameIndex);

	ErrCheck(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR, "Failed to acquire swapchain image!");

	// resetting the fence has been set after the result has been checked to
	// avoid a deadlock reset the fence to unsignaled state
	vkResetFences(m_Device->GetDevice(), 1, &m_InFlightFences[m_CurrentFrameIndex]);

	// begin command buffer
	m_ActiveCommandBuffer = m_CommandBuffers[m_CurrentFrameIndex];
	vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrameIndex], 0);
	VkCommandBufferBeginInfo cmdBuffBeginInfo{};
	cmdBuffBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	ErrCheck(vkBeginCommandBuffer(m_CommandBuffers[m_CurrentFrameIndex], &cmdBuffBeginInfo) != VK_SUCCESS,
		"Failed to begin recording command buffer!");

	// begin render pass
	// clear values for each attachment
	std::array<VkClearValue, 3> clearValues{};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	clearValues[2].color = clearValues[0].color;
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_SwapchainExtent.width);
	viewport.height = static_cast<float>(m_SwapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(m_ActiveCommandBuffer, 0, 1, &viewport);
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_SwapchainExtent;
	vkCmdSetScissor(m_ActiveCommandBuffer, 0, 1, &scissor);

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_RenderPass;
	renderPassBeginInfo.framebuffer = m_SwapchainFramebuffers[m_NextFrameIndex];
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = m_SwapchainExtent;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(m_ActiveCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Engine::EndScene()
{
	vkCmdEndRenderPass(m_ActiveCommandBuffer);
	ErrCheck(
		vkEndCommandBuffer(m_CommandBuffers[m_CurrentFrameIndex]) != VK_SUCCESS, "Failed to record command buffer!");

	std::array<VkPipelineStageFlags, 1> waitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrameIndex];
	submitInfo.pWaitDstStageMask = waitStages.data();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_CommandBuffers[m_CurrentFrameIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrameIndex];

	// signals the fence after executing the command buffer
	ErrCheck(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrameIndex])
				 != VK_SUCCESS,
		"Failed to submit draw command buffer!");

	std::array<VkSwapchainKHR, 1> swapchains{ m_Swapchain };
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrameIndex];
	presentInfo.swapchainCount = static_cast<uint32_t>(swapchains.size());
	presentInfo.pSwapchains = swapchains.data();
	presentInfo.pImageIndices = &m_NextFrameIndex;
	presentInfo.pResults = nullptr;

	vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);

	// update current frame index
	m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % Config::maxFramesInFlight;
}

void Engine::OnUiRender()
{
	ImGuiOverlay::Begin();

	ImGui::Begin("Profiler");
	ImGui::Text("%.2f ms/frame (%d fps)", (1000.0f / static_cast<float>(m_LastFps)), m_LastFps);
	ImGui::End();
	ImGuiOverlay::End(m_ActiveCommandBuffer);
}

float Engine::CalcFps()
{
	++m_FrameCounter;
	const std::chrono::time_point<std::chrono::high_resolution_clock> currentFrameTime =
		std::chrono::high_resolution_clock::now();

	const float deltatime =
		std::chrono::duration<float, std::chrono::milliseconds::period>(currentFrameTime - m_LastFrameTime).count();
	m_LastFrameTime = currentFrameTime;

	const float fpsTimer =
		std::chrono::duration<float, std::chrono::milliseconds::period>(currentFrameTime - m_FpsTimePoint).count();
	// calc fps every 1000ms
	if (fpsTimer > 1000.0f)
	{
		m_LastFps = static_cast<uint32_t>(static_cast<float>(m_FrameCounter) * (1000.0f / fpsTimer));
		m_FrameCounter = 0;
		m_FpsTimePoint = currentFrameTime;
	}

	return deltatime;
}

void Engine::CreateCommandPool()
{
	const VkCommandPoolCreateInfo commandPoolInfo = inits::CommandPoolCreateInfo(m_Device->GetQueueFamilyIndices());
	ErrCheck(vkCreateCommandPool(m_Device->GetDevice(), &commandPoolInfo, nullptr, &m_CommandPool) != VK_SUCCESS,
		"Failed to create command pool!");
}

void Engine::CreateDescriptorPool()
{
	const uint32_t poolSizeCount = 1000;
	VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, poolSizeCount },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, poolSizeCount },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, poolSizeCount },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, poolSizeCount },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, poolSizeCount },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, poolSizeCount },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, poolSizeCount },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, poolSizeCount },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, poolSizeCount },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, poolSizeCount },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, poolSizeCount },
	};

	const VkDescriptorPoolCreateInfo descriptorPoolInfo =
		inits::DescriptorPoolCreateInfo(poolSizes, std::size(poolSizes), poolSizeCount);
	ErrCheck(
		vkCreateDescriptorPool(m_Device->GetDevice(), &descriptorPoolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS,
		"Failed to create descriptor pool!");
}

void Engine::CreateSwapchain()
{
	const SwapchainSupportDetails swapchainSupport =
		Device::QuerySwapchainSupport(m_Device->GetPhysicalDevice(), m_Window->GetWindowSurface());
	const VkSurfaceFormatKHR surfaceFormat = utils::ChooseSurfaceFormat(swapchainSupport.formats);
	const VkPresentModeKHR presentMode = utils::ChoosePresentMode(swapchainSupport.presentModes);
	const VkExtent2D extent = utils::ChooseExtent(swapchainSupport.capabilities, BIND_FN(m_Window->GetFramebufferSize));

	uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
	if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
		imageCount = swapchainSupport.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR swapchainInfo{};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.surface = m_Window->GetWindowSurface();
	swapchainInfo.minImageCount = imageCount;
	swapchainInfo.imageFormat = surfaceFormat.format;
	swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainInfo.imageExtent = extent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (m_Device->GetQueueFamilyIndices().graphicsFamily.value()
		!= m_Device->GetQueueFamilyIndices().presentFamily.value())
	{
		uint32_t indicesArr[]{ m_Device->GetQueueFamilyIndices().graphicsFamily.value(),
			m_Device->GetQueueFamilyIndices().presentFamily.value() };
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainInfo.queueFamilyIndexCount = 2;
		swapchainInfo.pQueueFamilyIndices = static_cast<uint32_t*>(indicesArr);
	}
	else
	{
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	swapchainInfo.preTransform = swapchainSupport.capabilities.currentTransform;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.presentMode = presentMode;
	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

	ErrCheck(vkCreateSwapchainKHR(m_Device->GetDevice(), &swapchainInfo, nullptr, &m_Swapchain) != VK_SUCCESS,
		"Failed to create swapchain!");

	vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_Swapchain, &imageCount, nullptr);
	m_SwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_Swapchain, &imageCount, m_SwapchainImages.data());

	m_SwapchainImageFormat = surfaceFormat.format;
	m_SwapchainExtent = extent;

	m_AspectRatio = static_cast<float>(m_SwapchainExtent.width) / static_cast<float>(m_SwapchainExtent.height);
}

void Engine::CreateSwapchainImageViews()
{
	m_SwapchainImageViews.resize(m_SwapchainImages.size());

	for (size_t i = 0; i < m_SwapchainImageViews.size(); ++i)
	{
		m_SwapchainImageViews[i] = utils::CreateImageView(m_Device->GetDevice(),
			m_SwapchainImages[i],
			m_SwapchainImageFormat,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_ASPECT_COLOR_BIT,
			1,
			1);
	}
}

void Engine::RecreateSwapchain()
{
	while (m_Window->IsMinimized())
	{
		m_Window->WaitEvents();
	}

	vkDeviceWaitIdle(m_Device->GetDevice());
	CleanupSwapchain();

	CreateSwapchain();
	CreateSwapchainImageViews();
	CreateColorResource();
	CreateDepthResource();
	CreateFramebuffers();
}

void Engine::CleanupSwapchain()
{
	vkDestroyImageView(m_Device->GetDevice(), m_DepthImageView, nullptr);
	vkDestroyImage(m_Device->GetDevice(), m_DepthImage, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_DepthImageMemory, nullptr);

	vkDestroyImageView(m_Device->GetDevice(), m_ColorImageView, nullptr);
	vkDestroyImage(m_Device->GetDevice(), m_ColorImage, nullptr);
	vkFreeMemory(m_Device->GetDevice(), m_ColorImageMemory, nullptr);

	for (const auto& framebuffer : m_SwapchainFramebuffers)
		vkDestroyFramebuffer(m_Device->GetDevice(), framebuffer, nullptr);

	for (const auto& imageView : m_SwapchainImageViews)
		vkDestroyImageView(m_Device->GetDevice(), imageView, nullptr);

	// swapchain images are destroyed with `vkDestroySwapchainKHR()`
	vkDestroySwapchainKHR(m_Device->GetDevice(), m_Swapchain, nullptr);
}

void Engine::CreateRenderPass()
{
	const VkFormat depthFormat = utils::FindDepthFormat(m_Device->GetPhysicalDevice());

	// attachment descriptions
	const VkAttachmentDescription colorAttachment = inits::AttachmentDescription(m_SwapchainImageFormat,
		m_Device->GetMsaaSamples(),
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	const VkAttachmentDescription depthAttachment = inits::AttachmentDescription(depthFormat,
		m_Device->GetMsaaSamples(),
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	const VkAttachmentDescription colorResolveAttachment = inits::AttachmentDescription(
		m_SwapchainImageFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	std::array<VkAttachmentDescription, 3> attachments{ colorAttachment, depthAttachment, colorResolveAttachment };

	// attachment refrences
	const VkAttachmentReference colorRef = inits::AttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	const VkAttachmentReference depthRef =
		inits::AttachmentReference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	const VkAttachmentReference colorResolveRef =
		inits::AttachmentReference(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// subpass
	const VkSubpassDescription subpass = inits::SubpassDescription(1, &colorRef, &depthRef, &colorResolveRef);
	const VkSubpassDependency subpassDependency = inits::SubpassDependency(VK_SUBPASS_EXTERNAL,
		0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		0,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

	// render pass
	VkRenderPassCreateInfo renderPassInfo = inits::RenderPassCreateInfo(
		static_cast<uint32_t>(attachments.size()), attachments.data(), 1, &subpass, 1, &subpassDependency);
	ErrCheck(vkCreateRenderPass(m_Device->GetDevice(), &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS,
		"Failed to create render pass!");
}

void Engine::CreateColorResource()
{
	const VkFormat colorFormat = m_SwapchainImageFormat;
	const uint32_t miplevels = 1;

	utils::CreateImage(m_Device,
		m_SwapchainExtent.width,
		m_SwapchainExtent.height,
		miplevels,
		1,
		m_Device->GetMsaaSamples(),
		colorFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		0,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_ColorImage,
		m_ColorImageMemory);

	m_ColorImageView = utils::CreateImageView(m_Device->GetDevice(),
		m_ColorImage,
		colorFormat,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_ASPECT_COLOR_BIT,
		miplevels,
		1);
}

void Engine::CreateDepthResource()
{
	const VkFormat depthFormat = utils::FindDepthFormat(m_Device->GetPhysicalDevice());
	const uint32_t miplevels = 1;

	utils::CreateImage(m_Device,
		m_SwapchainExtent.width,
		m_SwapchainExtent.height,
		miplevels,
		1,
		m_Device->GetMsaaSamples(),
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		0,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_DepthImage,
		m_DepthImageMemory);

	m_DepthImageView = utils::CreateImageView(m_Device->GetDevice(),
		m_DepthImage,
		depthFormat,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		miplevels,
		1);
}

void Engine::CreateFramebuffers()
{
	m_SwapchainFramebuffers.resize(m_SwapchainImages.size());

	for (size_t i = 0; i < m_SwapchainImages.size(); ++i)
	{
		std::array<VkImageView, 3> fbAttachments{ m_ColorImageView, m_DepthImageView, m_SwapchainImageViews[i] };
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_RenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
		framebufferInfo.pAttachments = fbAttachments.data();
		framebufferInfo.width = m_SwapchainExtent.width;
		framebufferInfo.height = m_SwapchainExtent.height;
		framebufferInfo.layers = 1;

		ErrCheck(vkCreateFramebuffer(m_Device->GetDevice(), &framebufferInfo, nullptr, &m_SwapchainFramebuffers[i])
					 != VK_SUCCESS,
			"Failed to create framebuffer!");
	}
}

void Engine::CreateUniformBuffers()
{
	const VkDeviceSize bufferSize = SceneUBO::GetSize();
	m_SceneUniformBuffers.resize(Config::maxFramesInFlight);
	m_SceneUniformBufferMemory.resize(Config::maxFramesInFlight);

	const VkDeviceSize dBufferSize = MatrixUBO::GetSize();
	m_MatUniformBuffers.resize(Config::maxFramesInFlight);
	m_MatUniformBufferMemory.resize(Config::maxFramesInFlight);

	m_CubemapUniformBuffers.resize(Config::maxFramesInFlight);
	m_CubemapUniformBufferMem.resize(Config::maxFramesInFlight);

	for (uint64_t i = 0; i < Config::maxFramesInFlight; ++i)
	{
		utils::CreateBuffer(m_Device,
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			m_SceneUniformBuffers[i],
			m_SceneUniformBufferMemory[i]);

		utils::CreateBuffer(m_Device,
			dBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			m_MatUniformBuffers[i],
			m_MatUniformBufferMemory[i]);

		utils::CreateBuffer(m_Device,
			dBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			m_CubemapUniformBuffers[i],
			m_CubemapUniformBufferMem[i]);
	}
}

void Engine::CreateDescriptorSetLayout()
{
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	// matrices
	layoutBindings.push_back(
		inits::DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT));
	// scene lights and camera
	layoutBindings.push_back(
		inits::DescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS));
	// texture maps
	layoutBindings.push_back(inits::DescriptorSetLayoutBinding(2,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		static_cast<uint32_t>(m_TextureImages.size()),
		VK_SHADER_STAGE_FRAGMENT_BIT));

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	descriptorSetLayoutInfo.pBindings = layoutBindings.data();

	ErrCheck(
		vkCreateDescriptorSetLayout(m_Device->GetDevice(), &descriptorSetLayoutInfo, nullptr, &m_DescriptorSetLayout)
			!= VK_SUCCESS,
		"Failed to create descriptor set layout!");
}

void Engine::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> setLayouts{ Config::maxFramesInFlight, m_DescriptorSetLayout };
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocInfo.descriptorPool = m_DescriptorPool;
	descriptorSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(setLayouts.size());
	descriptorSetAllocInfo.pSetLayouts = setLayouts.data();

	m_DescriptorSets.resize(Config::maxFramesInFlight);
	ErrCheck(
		vkAllocateDescriptorSets(m_Device->GetDevice(), &descriptorSetAllocInfo, m_DescriptorSets.data()) != VK_SUCCESS,
		"Failed to allocate descriptor sets!");

	for (uint64_t i = 0; i < Config::maxFramesInFlight; ++i)
	{
		VkDescriptorBufferInfo dBufferInfo =
			inits::DescriptorBufferInfo(m_MatUniformBuffers[i], 0, MatrixUBO::GetSize());
		VkDescriptorBufferInfo bufferInfo =
			inits::DescriptorBufferInfo(m_SceneUniformBuffers[i], 0, SceneUBO::GetSize());

		std::vector<VkDescriptorImageInfo> textureImageInfos;
		for (const auto& imgView : m_TextureImageViews)
			textureImageInfos.push_back(inits::DescriptorImageInfo(m_TextureImageSampler, imgView));

		std::vector<VkWriteDescriptorSet> descWrites;
		descWrites.push_back(inits::WriteDescriptorSet(
			m_DescriptorSets[i], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, &dBufferInfo, nullptr));
		descWrites.push_back(inits::WriteDescriptorSet(
			m_DescriptorSets[i], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &bufferInfo, nullptr));
		descWrites.push_back(inits::WriteDescriptorSet(m_DescriptorSets[i],
			2,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			static_cast<uint32_t>(m_TextureImages.size()),
			nullptr,
			textureImageInfos.data()));

		vkUpdateDescriptorSets(
			m_Device->GetDevice(), static_cast<uint32_t>(descWrites.size()), descWrites.data(), 0, nullptr);
	}
}

void Engine::CreatePipelineLayout()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	ErrCheck(
		vkCreatePipelineLayout(m_Device->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS,
		"Failed to create pipeline layout!");
}

void Engine::CreatePipeline(const char* vertShaderPath, const char* fragShaderPath)
{
	// shader stages
	const Shader vertexShader{ m_Device->GetDevice(), vertShaderPath, ShaderType::VERTEX };
	const Shader fragmentShader{ m_Device->GetDevice(), fragShaderPath, ShaderType::FRAGMENT };
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{ vertexShader.GetShaderStage(),
		fragmentShader.GetShaderStage() };

	// vertex descriptions
	auto vertexBindingDesc = Vertex::GetBindingDescription();
	auto vertexAttrDesc = Vertex::GetAttributeDescription();

	// fixed functions
	const VkPipelineVertexInputStateCreateInfo vertexInputInfo = inits::PipelineVertexInputStateCreateInfo(
		1, &vertexBindingDesc, static_cast<uint32_t>(vertexAttrDesc.size()), vertexAttrDesc.data());
	const VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo =
		inits::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	const VkPipelineViewportStateCreateInfo viewportStateInfo = inits::PipelineViewportStateCreateInfo(1, 1);
	const VkPipelineRasterizationStateCreateInfo rasterizationStateInfo =
		inits::PipelineRasterizationStateCreateInfo(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	const VkPipelineMultisampleStateCreateInfo multisampleStateInfo =
		inits::PipelineMultisampleStateCreateInfo(VK_TRUE, m_Device->GetMsaaSamples(), 0.2f);
	const VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo =
		inits::PipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	const VkPipelineColorBlendStateCreateInfo colorBlendStateInfo =
		inits::PipelineColorBlendStateCreateInfo(colorBlendAttachment);

	// dynamic states
	std::array<VkDynamicState, 2> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateInfo =
		inits::PipelineDynamicStateCreateInfo(static_cast<uint32_t>(dynamicStates.size()), dynamicStates.data());

	// graphics pipeline
	VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
	graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// config all previos objects
	graphicsPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	graphicsPipelineInfo.pStages = shaderStages.data();
	graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	graphicsPipelineInfo.pViewportState = &viewportStateInfo;
	graphicsPipelineInfo.pRasterizationState = &rasterizationStateInfo;
	graphicsPipelineInfo.pMultisampleState = &multisampleStateInfo;
	graphicsPipelineInfo.pDepthStencilState = &depthStencilStateInfo;
	graphicsPipelineInfo.pColorBlendState = &colorBlendStateInfo;
	graphicsPipelineInfo.pDynamicState = &dynamicStateInfo;
	graphicsPipelineInfo.layout = m_PipelineLayout;
	graphicsPipelineInfo.renderPass = m_RenderPass;
	graphicsPipelineInfo.subpass = 0; // index of subpass
	graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineInfo.basePipelineIndex = -1;

	ErrCheck(
		vkCreateGraphicsPipelines(m_Device->GetDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &m_Pipeline)
			!= VK_SUCCESS,
		"Failed to create graphics pipeline!");
}

void Engine::CreateCommandBuffers()
{
	m_CommandBuffers.resize(Config::maxFramesInFlight);

	VkCommandBufferAllocateInfo cmdBuffAllocInfo{};
	cmdBuffAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBuffAllocInfo.commandPool = m_CommandPool;
	cmdBuffAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBuffAllocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

	ErrCheck(vkAllocateCommandBuffers(m_Device->GetDevice(), &cmdBuffAllocInfo, m_CommandBuffers.data()) != VK_SUCCESS,
		"Failed to allocate command buffers!");
}

void Engine::CreateVertexBuffer(const std::vector<Vertex>& vertices,
	VkBuffer& vertexBuffer,
	VkDeviceMemory& vertexBufferMemory)
{
	VkDeviceSize size = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer = nullptr;
	VkDeviceMemory stagingBufferMemory = nullptr;
	utils::CreateBuffer(Engine::GetInstance()->m_Device,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	void* data = nullptr;
	vkMapMemory(Engine::GetInstance()->m_Device->GetDevice(), stagingBufferMemory, 0, size, 0, &data);
	memcpy(data, vertices.data(), size);
	vkUnmapMemory(Engine::GetInstance()->m_Device->GetDevice(), stagingBufferMemory);

	utils::CreateBuffer(Engine::GetInstance()->m_Device,
		size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBuffer,
		vertexBufferMemory);

	utils::CopyBuffer(
		Engine::GetInstance()->m_Device, Engine::GetInstance()->m_CommandPool, stagingBuffer, vertexBuffer, size);

	vkFreeMemory(Engine::GetInstance()->m_Device->GetDevice(), stagingBufferMemory, nullptr);
	vkDestroyBuffer(Engine::GetInstance()->m_Device->GetDevice(), stagingBuffer, nullptr);
}

void Engine::CreateIndexBuffer(const std::vector<uint32_t>& indices,
	VkBuffer& indexBuffer,
	VkDeviceMemory& indexBufferMemory)
{
	VkDeviceSize size = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer = nullptr;
	VkDeviceMemory stagingBufferMemory = nullptr;
	utils::CreateBuffer(Engine::GetInstance()->m_Device,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	void* data = nullptr;
	vkMapMemory(Engine::GetInstance()->m_Device->GetDevice(), stagingBufferMemory, 0, size, 0, &data);
	memcpy(data, indices.data(), size);
	vkUnmapMemory(Engine::GetInstance()->m_Device->GetDevice(), stagingBufferMemory);

	utils::CreateBuffer(Engine::GetInstance()->m_Device,
		size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBuffer,
		indexBufferMemory);

	utils::CopyBuffer(
		Engine::GetInstance()->m_Device, Engine::GetInstance()->m_CommandPool, stagingBuffer, indexBuffer, size);

	vkFreeMemory(Engine::GetInstance()->m_Device->GetDevice(), stagingBufferMemory, nullptr);
	vkDestroyBuffer(Engine::GetInstance()->m_Device->GetDevice(), stagingBuffer, nullptr);
}

void Engine::CreateTextureImage(const char* texturePath,
	VkImage& textureImage,
	VkDeviceMemory& textureImageMem,
	VkFormat format,
	uint32_t& miplevels)
{
	int width = 0;
	int height = 0;
	int channels = 0;
	auto* imageData = stbi_load(texturePath, &width, &height, &channels, STBI_rgb_alpha);
	ErrCheck(!imageData, "Unable to load texture: \"{}\"; ERROR: {}", texturePath, stbi_failure_reason());

	VkDeviceSize size = static_cast<uint64_t>(width) * static_cast<uint64_t>(height) * 4;
	miplevels = static_cast<uint32_t>(std::log2(std::max(width, height))) + 1;

	VkBuffer stagingBuffer = nullptr;
	VkDeviceMemory stagingBufferMem = nullptr;
	utils::CreateBuffer(m_Device,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMem);

	void* data = nullptr;
	vkMapMemory(m_Device->GetDevice(), stagingBufferMem, 0, size, 0, &data);
	memcpy(data, imageData, size);
	vkUnmapMemory(m_Device->GetDevice(), stagingBufferMem);

	stbi_image_free(imageData);

	utils::CreateImage(m_Device,
		width,
		height,
		miplevels,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		0,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		textureImage,
		textureImageMem);

	utils::TransitionImageLayout(m_Device,
		m_CommandPool,
		textureImage,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		miplevels,
		1);

	utils::CopyBufferToImage(m_Device, m_CommandPool, stagingBuffer, textureImage, width, height, 1);

	utils::GenerateMipmaps(m_Device, m_CommandPool, textureImage, format, width, height, miplevels);

	vkFreeMemory(m_Device->GetDevice(), stagingBufferMem, nullptr);
	vkDestroyBuffer(m_Device->GetDevice(), stagingBuffer, nullptr);
}

void Engine::CreateTextureSampler()
{
	VkPhysicalDeviceProperties devProp = m_Device->GetDeviceProperties();

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = devProp.limits.maxSamplerAnisotropy;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	ErrCheck(vkCreateSampler(m_Device->GetDevice(), &samplerInfo, nullptr, &m_TextureImageSampler) != VK_SUCCESS,
		"Failed to create texture sampler!");
}

void Engine::CreateCubemap(const std::array<const char*, 6>& cubemapPaths,
	VkFormat format,
	VkImageAspectFlags aspectFlags,
	uint32_t& miplevels,
	VkImage& cubemapImage,
	VkDeviceMemory& cubemapImageMem,
	VkImageView& cubemapImageView)
{
	// width, height, and channels should be the same for all 6 images
	int32_t width = 0;
	int32_t height = 0;
	int32_t channels = 0;
	constexpr int32_t numImages = 6;

	stbi_uc* imageData[numImages]{};

	for (int32_t i = 0; i < numImages; ++i)
	{
		imageData[i] = stbi_load(cubemapPaths[i], &width, &height, &channels, STBI_rgb_alpha);
		ErrCheck(!imageData[i], "Unable to load texture: \"{}\"", cubemapPaths[i]);
	}

	uint64_t imageSize = static_cast<uint64_t>(width) * static_cast<uint64_t>(height) * 4;
	VkDeviceSize bufferSize = imageSize * numImages;
	miplevels = 1;

	VkBuffer stagingBuffer = nullptr;
	VkDeviceMemory stagingBufferMem = nullptr;
	utils::CreateBuffer(m_Device,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMem);

	for (uint64_t i = 0; i < numImages; ++i)
	{
		void* data = nullptr;
		vkMapMemory(m_Device->GetDevice(), stagingBufferMem, imageSize * i, imageSize, 0, &data);
		memcpy(data, imageData[i], imageSize);
		vkUnmapMemory(m_Device->GetDevice(), stagingBufferMem);

		stbi_image_free(imageData[i]);
		imageData[i] = nullptr;
	}


	utils::CreateImage(m_Device,
		width,
		height,
		miplevels,
		numImages,
		VK_SAMPLE_COUNT_1_BIT,
		format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		cubemapImage,
		cubemapImageMem);

	utils::TransitionImageLayout(m_Device,
		m_CommandPool,
		cubemapImage,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		miplevels,
		numImages);

	utils::CopyBufferToImage(m_Device, m_CommandPool, stagingBuffer, cubemapImage, width, height, numImages);

	utils::TransitionImageLayout(m_Device,
		m_CommandPool,
		cubemapImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		miplevels,
		numImages);

	vkFreeMemory(m_Device->GetDevice(), stagingBufferMem, nullptr);
	vkDestroyBuffer(m_Device->GetDevice(), stagingBuffer, nullptr);

	cubemapImageView = utils::CreateImageView(
		m_Device->GetDevice(), cubemapImage, format, VK_IMAGE_VIEW_TYPE_CUBE, aspectFlags, miplevels, numImages);
}

void Engine::CreateCubemapDescriptorSetLayout()
{
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	// matrices
	layoutBindings.push_back(
		inits::DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT));
	// skybox textures
	layoutBindings.push_back(inits::DescriptorSetLayoutBinding(
		1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT));

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	descriptorSetLayoutInfo.pBindings = layoutBindings.data();

	ErrCheck(vkCreateDescriptorSetLayout(
				 m_Device->GetDevice(), &descriptorSetLayoutInfo, nullptr, &m_CubemapDescriptorSetLayout)
				 != VK_SUCCESS,
		"Failed to create descriptor set layout!");
}

void Engine::CreateCubemapDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> setLayouts{ Config::maxFramesInFlight, m_CubemapDescriptorSetLayout };
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocInfo.descriptorPool = m_DescriptorPool;
	descriptorSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(setLayouts.size());
	descriptorSetAllocInfo.pSetLayouts = setLayouts.data();

	m_CubemapDescriptorSets.resize(Config::maxFramesInFlight);
	ErrCheck(vkAllocateDescriptorSets(m_Device->GetDevice(), &descriptorSetAllocInfo, m_CubemapDescriptorSets.data())
				 != VK_SUCCESS,
		"Failed to allocate descriptor sets!");

	for (uint64_t i = 0; i < Config::maxFramesInFlight; ++i)
	{
		VkDescriptorBufferInfo dBufferInfo =
			inits::DescriptorBufferInfo(m_CubemapUniformBuffers[i], 0, MatrixUBO::GetSize());
		VkDescriptorImageInfo cubemapImageInfos = inits::DescriptorImageInfo(m_TextureImageSampler, m_CubemapImageView);

		std::vector<VkWriteDescriptorSet> descWrites;
		descWrites.push_back(inits::WriteDescriptorSet(
			m_CubemapDescriptorSets[i], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, &dBufferInfo, nullptr));
		descWrites.push_back(inits::WriteDescriptorSet(
			m_CubemapDescriptorSets[i], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, nullptr, &cubemapImageInfos));

		vkUpdateDescriptorSets(
			m_Device->GetDevice(), static_cast<uint32_t>(descWrites.size()), descWrites.data(), 0, nullptr);
	}
}

void Engine::CreateCubemapPipelineLayout()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_CubemapDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	ErrCheck(vkCreatePipelineLayout(m_Device->GetDevice(), &pipelineLayoutInfo, nullptr, &m_CubemapPipelineLayout)
				 != VK_SUCCESS,
		"Failed to create pipeline layout!");
}

void Engine::CreateCubemapPipeline(const char* vertShaderPath, const char* fragShaderPath)
{
	// shader stages
	const Shader vertexShader{ m_Device->GetDevice(), vertShaderPath, ShaderType::VERTEX };
	const Shader fragmentShader{ m_Device->GetDevice(), fragShaderPath, ShaderType::FRAGMENT };
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{ vertexShader.GetShaderStage(),
		fragmentShader.GetShaderStage() };

	// vertex descriptions
	auto vertexBindingDesc = Vertex::GetBindingDescription();
	auto vertexAttrDesc = Vertex::GetAttributeDescription();

	// fixed functions
	const VkPipelineVertexInputStateCreateInfo vertexInputInfo = inits::PipelineVertexInputStateCreateInfo(
		1, &vertexBindingDesc, static_cast<uint32_t>(vertexAttrDesc.size()), vertexAttrDesc.data());
	const VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo =
		inits::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	const VkPipelineViewportStateCreateInfo viewportStateInfo = inits::PipelineViewportStateCreateInfo(1, 1);
	const VkPipelineRasterizationStateCreateInfo rasterizationStateInfo =
		inits::PipelineRasterizationStateCreateInfo(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	const VkPipelineMultisampleStateCreateInfo multisampleStateInfo =
		inits::PipelineMultisampleStateCreateInfo(VK_TRUE, m_Device->GetMsaaSamples(), 0.2f);
	const VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo =
		inits::PipelineDepthStencilStateCreateInfo(VK_TRUE,
			VK_TRUE,
			VK_COMPARE_OP_LESS_OR_EQUAL); // less or equal because the depth buffer for skybox will be filled with 1.0

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	const VkPipelineColorBlendStateCreateInfo colorBlendStateInfo =
		inits::PipelineColorBlendStateCreateInfo(colorBlendAttachment);

	// dynamic states
	std::array<VkDynamicState, 2> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateInfo =
		inits::PipelineDynamicStateCreateInfo(static_cast<uint32_t>(dynamicStates.size()), dynamicStates.data());

	// graphics pipeline
	VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
	graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// config all previos objects
	graphicsPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	graphicsPipelineInfo.pStages = shaderStages.data();
	graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	graphicsPipelineInfo.pViewportState = &viewportStateInfo;
	graphicsPipelineInfo.pRasterizationState = &rasterizationStateInfo;
	graphicsPipelineInfo.pMultisampleState = &multisampleStateInfo;
	graphicsPipelineInfo.pDepthStencilState = &depthStencilStateInfo;
	graphicsPipelineInfo.pColorBlendState = &colorBlendStateInfo;
	graphicsPipelineInfo.pDynamicState = &dynamicStateInfo;
	graphicsPipelineInfo.layout = m_CubemapPipelineLayout;
	graphicsPipelineInfo.renderPass = m_RenderPass;
	graphicsPipelineInfo.subpass = 0; // index of subpass
	graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineInfo.basePipelineIndex = -1;

	ErrCheck(vkCreateGraphicsPipelines(
				 m_Device->GetDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &m_CubemapPipeline)
				 != VK_SUCCESS,
		"Failed to create graphics pipeline!");
}

void Engine::CreateCubemapVertexBuffer()
{
	m_CubemapVertices = utils::GenerateSkyboxData();
	VkDeviceSize size = sizeof(m_CubemapVertices[0]) * m_CubemapVertices.size();

	VkBuffer stagingBuffer = nullptr;
	VkDeviceMemory stagingBufferMemory = nullptr;
	utils::CreateBuffer(m_Device,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	void* data = nullptr;
	vkMapMemory(m_Device->GetDevice(), stagingBufferMemory, 0, size, 0, &data);
	memcpy(data, m_CubemapVertices.data(), size);
	vkUnmapMemory(m_Device->GetDevice(), stagingBufferMemory);

	utils::CreateBuffer(m_Device,
		size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_CubemapVertexBuffer,
		m_CubemapVertexBufferMem);

	utils::CopyBuffer(m_Device, m_CommandPool, stagingBuffer, m_CubemapVertexBuffer, size);

	vkFreeMemory(m_Device->GetDevice(), stagingBufferMemory, nullptr);
	vkDestroyBuffer(m_Device->GetDevice(), stagingBuffer, nullptr);
}

void Engine::CreateSyncObjects()
{
	m_ImageAvailableSemaphores.resize(Config::maxFramesInFlight);
	m_RenderFinishedSemaphores.resize(Config::maxFramesInFlight);
	m_InFlightFences.resize(Config::maxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// create the fence in signaled state so that the first frame doesnt have to wait
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32_t i = 0; i < Config::maxFramesInFlight; ++i)
	{
		ErrCheck(
			vkCreateSemaphore(m_Device->GetDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i])
					!= VK_SUCCESS
				|| vkCreateSemaphore(m_Device->GetDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i])
					   != VK_SUCCESS
				|| vkCreateFence(m_Device->GetDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS,
			"Failed to create synchronization objects!");
	}
}

// event callbacks
void Engine::ProcessInput()
{
	// forward input data to ImGui first
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse || io.WantCaptureKeyboard)
		return;

	if (Input::IsMouseButtonPressed(Mouse::BUTTON_1))
	{
		// hide cursor when moving camera
		m_Window->HideCursor();
	}
	else if (Input::IsMouseButtonReleased(Mouse::BUTTON_1))
	{
		// unhide cursor when camera stops moving
		m_Window->ShowCursor();
	}
}

void Engine::OnCloseEvent()
{
	m_IsRunning = false;
}

void Engine::OnResizeEvent(int width, int height)
{
	RecreateSwapchain();
	m_Camera->SetAspectRatio(m_AspectRatio);
}

void Engine::OnMouseMoveEvent(double xpos, double ypos)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse)
		return;

	m_Camera->OnMouseMove(xpos, ypos);
}

void Engine::OnKeyEvent(int key, int scancode, int action, int mods)
{
	// quits the application
	// works even when the ui is in focus
	if (Input::IsKeyPressed(Key::LEFT_CONTROL) && Input::IsKeyPressed(Key::Q))
		m_IsRunning = false;

	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard)
		return;
}