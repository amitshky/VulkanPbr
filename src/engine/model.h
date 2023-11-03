#pragma once

#include <vector>
#include <string>
#include "assimp/scene.h"
#include "engine/types.h"


class Model
{
public:
	explicit Model(const char* path, bool flipUVs = false);

	[[nodiscard]] inline std::pair<std::vector<Vertex>, std::vector<uint32_t>> GetModelData() const
	{
		return { m_Vertices, m_Indices };
	}

	// [[nodiscard]] inline std::vector<Vertex> GetModelData() const { return m_Vertices; }
	[[nodiscard]] inline std::vector<std::string> GetTexturePaths() const { return m_LoadedTextures; }

private:
	void LoadModel(const std::string& path, bool flipUVs);
	void ProcessNode(aiNode* node, const aiScene* scene);
	void ProcessMesh(aiMesh* mesh, const aiScene* scene);
	void LoadTextures(aiMaterial* material, aiTextureType type);

	std::string m_Directory;
	std::vector<Vertex> m_Vertices;
	std::vector<uint32_t> m_Indices;
	std::vector<std::string> m_LoadedTextures;
};