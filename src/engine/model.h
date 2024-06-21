#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "assimp/scene.h"
#include "engine/types.h"


class Model
{
public:
	explicit Model(const char* path, bool loadPbrTextures = true, bool flipUVs = false);

	void Draw(VkCommandBuffer activeCommandBuffer);
	void Cleanup(VkDevice deviceVk);

	// [[nodiscard]] inline std::pair<std::vector<Vertex>, std::vector<uint32_t>> GetModelData()
	// const
	// {
	// 	return { m_Vertices, m_Indices };
	// }

	[[nodiscard]] inline std::vector<VkBuffer> GetVertexBuffers() const { return m_VertexBuffers; }
	[[nodiscard]] inline std::vector<VkDeviceMemory> GetVertexBufferMemories() const
	{
		return m_VertexBufferMems;
	}

	[[nodiscard]] inline std::vector<VkBuffer> GetIndexBuffers() const { return m_IndexBuffers; }
	[[nodiscard]] inline std::vector<VkDeviceMemory> GetIndexBufferMemories() const
	{
		return m_IndexBufferMems;
	}

	[[nodiscard]] inline std::vector<std::string> GetTexturePaths() const
	{
		return m_LoadedTextures;
	}


private:
	void LoadModel(const std::string& path, bool flipUVs);
	void ProcessNode(aiNode* node, const aiScene* scene);
	void ProcessMesh(aiMesh* mesh, const aiScene* scene);
	void LoadTextures(aiMaterial* material, aiTextureType type);

	bool m_LoadPbrTextures;
	std::string m_Directory;

	std::vector<uint64_t> m_VertexCounts;
	std::vector<uint64_t> m_IndexCounts;
	std::vector<VkBuffer> m_VertexBuffers;
	std::vector<VkDeviceMemory> m_VertexBufferMems;
	std::vector<VkBuffer> m_IndexBuffers;
	std::vector<VkDeviceMemory> m_IndexBufferMems;

	std::vector<std::string> m_LoadedTextures;
};