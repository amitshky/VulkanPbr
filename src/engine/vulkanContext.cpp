#include "engine/vulkanContext.h"

#include "core/core.h"
#include "engine/types.h"
#include "engine/initializers.h"
#include "utils/utils.h"


VulkanContext::VulkanContext(const char* title)
{
	Init(title);
}

VulkanContext::~VulkanContext()
{
	Cleanup();
}

void VulkanContext::Init(const char* title)
{
	// init vulkan instance
	ErrCheck(Config::enableValidationLayers && !utils::CheckValidationLayerSupport(),
		"Validation layers requested, but not available!");

	std::vector<const char*> extensions = utils::GetRequiredExtensions();

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = title;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0; // the highest version the application is designed to use

	VkInstanceCreateInfo instanceInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceInfo.ppEnabledExtensionNames = extensions.data();
	if (Config::enableValidationLayers)
	{
		m_DebugMessengerInfo = inits::DebugMessengerCreateInfo(utils::DebugCallback);
		instanceInfo.enabledLayerCount = static_cast<uint32_t>(Config::validationLayers.size());
		instanceInfo.ppEnabledLayerNames = Config::validationLayers.data();
		instanceInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&m_DebugMessengerInfo);
	}
	else
	{
		instanceInfo.enabledLayerCount = 0;
		instanceInfo.pNext = nullptr;
	}

	ErrCheck(
		vkCreateInstance(&instanceInfo, nullptr, &m_VulkanInstance) != VK_SUCCESS, "Failed to create Vulkan instance!");

	// setup debug messenger
	if (Config::enableValidationLayers)
	{
		ErrCheck(
			inits::CreateDebugUtilsMessengerEXT(m_VulkanInstance, &m_DebugMessengerInfo, nullptr, &m_DebugMessenger)
				!= VK_SUCCESS,
			"Failed to setup debug messenger!");
	}
}

void VulkanContext::Cleanup()
{
	if (Config::enableValidationLayers)
		inits::DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);
	vkDestroyInstance(m_VulkanInstance, nullptr);
}