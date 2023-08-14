#pragma once

#include <array>
#include <vector>
#include <optional>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

struct Config
{
public:
	const static bool enableValidationLayers;
	const static uint32_t maxFramesInFlight;
	const static std::array<const char*, 1> validationLayers;
	const static std::array<const char*, 1> deviceExtensions;
};

struct QueueFamilyIndices
{
	// std::optional contains no value until you assign something to it
	// we can check if it contains a value by calling has_value()
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	[[nodiscard]] inline bool IsComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct SwapchainCreateDetails
{
	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
	uint32_t imageCount;
	VkSurfaceKHR windowSurface;
	VkSurfaceTransformFlagBitsKHR currentTransform;
	QueueFamilyIndices queueFamilyIndices;
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDesc{};
		bindingDesc.binding = 0;
		bindingDesc.stride = sizeof(Vertex);
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDesc;
	}

	static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescription()
	{
		std::array<VkVertexInputAttributeDescription, 3> attrDesc{};
		// for position
		attrDesc[0].location = 0;
		attrDesc[0].binding = 0;
		attrDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrDesc[0].offset = offsetof(Vertex, pos);
		// for color
		attrDesc[1].location = 1;
		attrDesc[1].binding = 0;
		attrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrDesc[1].offset = offsetof(Vertex, normal);
		// for texture coordinates
		attrDesc[2].location = 2;
		attrDesc[2].binding = 0;
		attrDesc[2].format = VK_FORMAT_R32G32_SFLOAT;
		attrDesc[2].offset = offsetof(Vertex, texCoord);

		return attrDesc;
	}

	// for hash function
	bool operator==(const Vertex& other) const
	{
		return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
	}
};

// hash function
namespace std {
template<>
struct hash<Vertex>
{
	size_t operator()(Vertex const& vertex) const
	{
		return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1)
			   ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
	}
};
} // namespace std

struct UniformBufferObject
{
	alignas(16) glm::vec3 cameraPos;
	alignas(16) glm::vec3 albedo;
	alignas(4) float metallic;
	alignas(4) float roughness;
	alignas(4) float ao;
};

struct DynamicUBO
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 viewProj;
	alignas(16) glm::mat4 normalMat;

	// NOTE: if this struct is updated don't forget to update this function as well
	[[nodiscard]] static inline uint64_t GetSize() { return sizeof(glm::mat4) * 3ull; }
};
