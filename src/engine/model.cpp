#include "engine/model.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "core/core.h"
#include "core/logger.h"
#include "engine/engine.h"


Model::Model(const char* path, bool loadPbrTextures, bool flipUVs)
	: m_LoadPbrTextures{ loadPbrTextures }
{
	Logger::Info("Loading model \"{}\"", path);
	LoadModel(path, flipUVs);
	Logger::Info("Model loaded");
}

void Model::Draw(VkCommandBuffer activeCommandBuffer)
{
	VkDeviceSize offset = 0;
	// vertexCounts, indexCounts have the same size
	for (uint64_t i = 0; i < m_VertexCounts.size(); ++i)
	{
		vkCmdBindVertexBuffers(activeCommandBuffer, 0, 1, &m_VertexBuffers[i], &offset);
		vkCmdBindIndexBuffer(activeCommandBuffer, m_IndexBuffers[i], 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(activeCommandBuffer, static_cast<uint32_t>(m_IndexCounts[i]), 1, 0, 0, 0);
	}
}

void Model::Cleanup(VkDevice deviceVk)
{
	for (uint64_t i = 0; i < m_VertexCounts.size(); ++i)
	{
		vkDestroyBuffer(deviceVk, m_IndexBuffers[i], nullptr);
		vkDestroyBuffer(deviceVk, m_VertexBuffers[i], nullptr);
		vkFreeMemory(deviceVk, m_IndexBufferMems[i], nullptr);
		vkFreeMemory(deviceVk, m_VertexBufferMems[i], nullptr);
	}
}

void Model::LoadModel(const std::string& path, bool flipUVs)
{
	uint32_t pFlags = aiProcess_Triangulate | aiProcess_CalcTangentSpace;
	if (flipUVs)
		pFlags |= aiProcess_FlipUVs;

	Assimp::Importer importer{};
	const aiScene* scene = importer.ReadFile(path, pFlags);
	ErrCheck(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode,
		"Failed to load model! {}",
		importer.GetErrorString());

	m_Directory = path.substr(0, path.find_last_of('/'));

	ProcessNode(scene->mRootNode, scene);
}

void Model::ProcessNode(aiNode* node, const aiScene* scene)
{
	for (uint32_t i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh, scene);
	}

	for (uint32_t i = 0; i < node->mNumChildren; ++i)
		ProcessNode(node->mChildren[i], scene);
}

void Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Vertex> vertices{};
	vertices.reserve(mesh->mNumVertices);
	std::vector<uint32_t> indices{};
	indices.reserve(mesh->mNumFaces); // this will be more than `mNumFaces`

	// process vertices
	for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
	{
		Vertex vertex{};
		glm::vec3 position{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
		vertex.pos = position;

		if (mesh->HasNormals())
			vertex.normal =
				glm::vec3{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

		if (mesh->mTextureCoords[0]) // check if the mesh has texture coords
			vertex.texCoord =
				glm::vec2{ mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

		if (mesh->HasTangentsAndBitangents())
			vertex.tangent =
				glm::vec3{ mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };

		vertices.push_back(vertex);
	}

	VkBuffer vertexBuffer = nullptr;
	VkDeviceMemory vertexBufferMemory = nullptr;
	Engine::CreateVertexBuffer(vertices, vertexBuffer, vertexBufferMemory);
	m_VertexBuffers.push_back(vertexBuffer);
	m_VertexBufferMems.push_back(vertexBufferMemory);
	m_VertexCounts.push_back(vertices.size());

	// process indices
	for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace face = mesh->mFaces[i];
		for (uint32_t j = 0; j < face.mNumIndices; ++j)
			indices.push_back(face.mIndices[j]);
	}

	VkBuffer indexBuffer = nullptr;
	VkDeviceMemory indexBufferMemory = nullptr;
	Engine::CreateIndexBuffer(indices, indexBuffer, indexBufferMemory);
	m_IndexBuffers.push_back(indexBuffer);
	m_IndexBufferMems.push_back(indexBufferMemory);
	m_IndexCounts.push_back(indices.size());

	// process materials
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		if (m_LoadPbrTextures)
		{
			LoadTextures(material, aiTextureType_DIFFUSE); // albedo
			LoadTextures(material, aiTextureType_DIFFUSE_ROUGHNESS); // roughness
			LoadTextures(material, aiTextureType_METALNESS); // metallic
			LoadTextures(material, aiTextureType_AMBIENT_OCCLUSION); // ao
			LoadTextures(material, aiTextureType_NORMALS); // normal
		}
		else
		{
			LoadTextures(material, aiTextureType_DIFFUSE);
			LoadTextures(material, aiTextureType_SPECULAR);
		}
	}
}

void Model::LoadTextures(aiMaterial* material, aiTextureType type)
{
	uint32_t textureCount = material->GetTextureCount(type);
	if (textureCount == 0)
	{
		// fallback textures if the texture could not be loaded
		const char* texPath = "assets/textures/checkerboard.png";
		const char* aoPath = "assets/textures/white.png";
		const char* normalPath = "assets/textures/normal.png";

		switch (type)
		{
		case aiTextureType_DIFFUSE:
			Logger::Warn("Fallback Diffuse texture loaded: \"{}\"", texPath);
			m_LoadedTextures.emplace_back(texPath);
			break;

		case aiTextureType_DIFFUSE_ROUGHNESS:
			Logger::Warn("Fallback Roughness texture loaded: \"{}\"", texPath);
			m_LoadedTextures.emplace_back(texPath);
			break;

		case aiTextureType_METALNESS:
			Logger::Warn("Fallback Metallic texture loaded: \"{}\"", texPath);
			m_LoadedTextures.emplace_back(texPath);
			break;

		case aiTextureType_AMBIENT_OCCLUSION:
			Logger::Warn("Fallback AO texture loaded: \"{}\"", aoPath);
			m_LoadedTextures.emplace_back(aoPath);
			break;

		case aiTextureType_NORMALS:
			Logger::Warn("Fallback Normal texture loaded: \"{}\"", normalPath);
			m_LoadedTextures.emplace_back(normalPath);
			break;

		case aiTextureType_SPECULAR:
			Logger::Warn("Fallback Specular texture loaded: \"{}\"", texPath);
			m_LoadedTextures.emplace_back(texPath);
			break;

		default:
			Logger::Warn("Fallback texture loaded: \"{}\"", texPath);
			m_LoadedTextures.emplace_back(texPath);
			break;
		}

		return;
	}

	for (uint32_t i = 0; i < textureCount; ++i)
	{
		aiString filename;
		bool skip = false;

		material->GetTexture(type, i, &filename);
		std::string texturePath = m_Directory + '/' + filename.C_Str();

		// check if the texture has already been loaded
		for (const auto& texture : m_LoadedTextures)
		{
			if (texturePath == texture)
			{
				skip = true;
				break;
			}
		}

		if (!skip)
		{
			m_LoadedTextures.push_back(texturePath);
			Logger::Info("    Loaded texture: \"{}\"", texturePath.c_str());
		}
	}
}