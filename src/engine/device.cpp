#include "engine/device.h"

#include <vector>
#include <set>
#include "core/core.h"
#include "utils/utils.h"


Device::Device(VkInstance vulkanInstance, VkSurfaceKHR windowSurface)
{
	Init(vulkanInstance, windowSurface);
}

Device::~Device()
{
	Cleanup();
}

void Device::Init(VkInstance vulkanInstance, VkSurfaceKHR windowSurface)
{
	PickPhysicalDevice(vulkanInstance, windowSurface);
	CreateLogicalDevice(windowSurface);
}

void Device::Cleanup()
{
	vkDestroyDevice(m_VulkanDevice, nullptr);
}

void Device::PickPhysicalDevice(VkInstance vulkanInstance, VkSurfaceKHR windowSurface)
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(vulkanInstance, &physicalDeviceCount, nullptr);

	ErrCheck(physicalDeviceCount == 0, "Failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> physicalDevices{ physicalDeviceCount };
	vkEnumeratePhysicalDevices(vulkanInstance, &physicalDeviceCount, physicalDevices.data());

	for (const auto& phyDevice : physicalDevices)
	{
		if (IsDeviceSuitable(phyDevice, windowSurface))
		{
			m_PhysicalDevice = phyDevice;
			vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_PhysicalDeviceFeatures);
			vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalDeviceProperties);
			vkGetPhysicalDeviceMemoryProperties(
				m_PhysicalDevice, &m_PhysicalDeviceMemoryProperties);
			m_MsaaSamples = GetMaxUsableSampleCount(m_PhysicalDeviceProperties);
			break;
		}
	}

	ErrCheck(m_PhysicalDevice == VK_NULL_HANDLE, "Failed to find a suitable GPU!");

	Logger::Info(
		"Physical device info:\n"
		"    Device name: {}",
		m_PhysicalDeviceProperties.deviceName);
}

void Device::CreateLogicalDevice(VkSurfaceKHR windowSurface)
{
	// create queue
	m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice, windowSurface);

	// we have multiple queues so we create a set of unique queue families
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
	const std::set<uint32_t> uniqueQueueFamilies = { m_QueueFamilyIndices.graphicsFamily.value(),
		m_QueueFamilyIndices.presentFamily.value() };

	const float queuePriority = 1.0f;
	for (const auto& queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = queueFamily;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueInfo);
	}

	// specify used device features
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE; // enable sample shading

	// create logical device
	VkDeviceCreateInfo deviceInfo{};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceInfo.pEnabledFeatures = &deviceFeatures;
	deviceInfo.enabledExtensionCount = static_cast<uint32_t>(Config::deviceExtensions.size());
	deviceInfo.ppEnabledExtensionNames = Config::deviceExtensions.data();
	if (Config::enableValidationLayers)
	{
		deviceInfo.enabledLayerCount = static_cast<uint32_t>(Config::validationLayers.size());
		deviceInfo.ppEnabledLayerNames = Config::validationLayers.data();
	}
	else
	{
		deviceInfo.enabledLayerCount = 0;
	}

	ErrCheck(vkCreateDevice(m_PhysicalDevice, &deviceInfo, nullptr, &m_VulkanDevice) != VK_SUCCESS,
		"Failed to create logcial device!");

	// get the queue handle
	vkGetDeviceQueue(
		m_VulkanDevice, m_QueueFamilyIndices.graphicsFamily.value(), 0, &m_GraphicsQueue);
	vkGetDeviceQueue(
		m_VulkanDevice, m_QueueFamilyIndices.presentFamily.value(), 0, &m_PresentQueue);
}

VkSampleCountFlagBits Device::GetMaxUsableSampleCount(VkPhysicalDeviceProperties properties)
{
	const uint64_t counts = properties.limits.framebufferColorSampleCounts
							& properties.limits.framebufferDepthSampleCounts;

	if ((counts & static_cast<uint64_t>(VK_SAMPLE_COUNT_64_BIT)) != 0u)
		return VK_SAMPLE_COUNT_64_BIT;

	if ((counts & static_cast<uint64_t>(VK_SAMPLE_COUNT_32_BIT)) != 0u)
		return VK_SAMPLE_COUNT_32_BIT;

	if ((counts & static_cast<uint64_t>(VK_SAMPLE_COUNT_16_BIT)) != 0u)
		return VK_SAMPLE_COUNT_16_BIT;

	if ((counts & static_cast<uint64_t>(VK_SAMPLE_COUNT_8_BIT)) != 0u)
		return VK_SAMPLE_COUNT_8_BIT;

	if ((counts & static_cast<uint64_t>(VK_SAMPLE_COUNT_4_BIT)) != 0u)
		return VK_SAMPLE_COUNT_4_BIT;

	if ((counts & static_cast<uint64_t>(VK_SAMPLE_COUNT_2_BIT)) != 0u)
		return VK_SAMPLE_COUNT_2_BIT;

	return VK_SAMPLE_COUNT_1_BIT;
}

SwapchainSupportDetails Device::QuerySwapchainSupport(VkPhysicalDevice physicalDevice,
	VkSurfaceKHR windowSurface)
{
	// Simply checking swapchain availability is not enough,
	// we need to check if it is supported by our window surface or not
	// We need to check for:
	// * basic surface capabilities (min/max number of images in swap chain)
	// * surface formats (pixel format and color space)
	// * available presentation modes
	SwapchainSupportDetails swapchainDetails{};

	// query surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		physicalDevice, windowSurface, &swapchainDetails.capabilities);

	// query surface format
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		swapchainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			physicalDevice, windowSurface, &formatCount, swapchainDetails.formats.data());
	}

	// query supported presentation modes
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(
		physicalDevice, windowSurface, &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		swapchainDetails.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			physicalDevice, windowSurface, &presentModeCount, swapchainDetails.presentModes.data());
	}

	return swapchainDetails;
}

bool Device::IsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface)
{
	QueueFamilyIndices indicies = FindQueueFamilies(physicalDevice, windowSurface);

	// checking for extension availability like swapchain extension availability
	bool extensionsSupported = utils::CheckDeviceExtensionSupport(physicalDevice);

	// checking if swapchain is supported by window surface
	bool swapchainAdequate = false;
	if (extensionsSupported)
	{
		SwapchainSupportDetails swapchainSupport =
			QuerySwapchainSupport(physicalDevice, windowSurface);
		swapchainAdequate =
			!swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	return indicies.IsComplete() && extensionsSupported && swapchainAdequate
		   && (supportedFeatures.samplerAnisotropy != 0u);
}

QueueFamilyIndices Device::FindQueueFamilies(VkPhysicalDevice physicalDevice,
	VkSurfaceKHR windowSurface)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies{ queueFamilyCount };
	vkGetPhysicalDeviceQueueFamilyProperties(
		physicalDevice, &queueFamilyCount, queueFamilies.data());

	// find a queue that supports graphics commands
	for (int i = 0; i < queueFamilies.size(); ++i)
	{
		if ((queueFamilies[i].queueFlags & static_cast<uint64_t>(VK_QUEUE_GRAPHICS_BIT)) != 0u)
			indices.graphicsFamily = i;

		// check for queue family compatible for presentation
		// the graphics queue and the presentation queue might end up being the
		// same but we treat them as separate queues
		auto presentSupport = static_cast<VkBool32>(false);
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, windowSurface, &presentSupport);

		if (presentSupport != 0u)
			indices.presentFamily = i;

		if (indices.IsComplete())
			break;
	}

	return indices;
}
