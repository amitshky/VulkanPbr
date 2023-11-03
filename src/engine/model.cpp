#include "engine/model.h"

#include "core/core.h"
#include "core/logger.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"


Model::Model(const char* path, bool flipUVs)
{
	Logger::Info("Loading model \"{}\"", path);
	LoadModel(path, flipUVs);
	Logger::Info("Model loaded");
}

void Model::LoadModel(const std::string& path, bool flipUVs)
{
	uint32_t pFlags = aiProcess_Triangulate;
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
	// process vertices
	for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
	{
		Vertex vertex{};
		glm::vec3 position{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
		vertex.pos = position;

		if (mesh->HasNormals())
		{
			glm::vec3 normal{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
			vertex.normal = normal;
		}
		else
			vertex.normal = glm::vec3(0.0f);

		if (mesh->mTextureCoords[0]) // check if the mesh has texture coords
		{
			glm::vec2 texCoords{ mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
			vertex.texCoord = texCoords;
		}
		else
			vertex.texCoord = glm::vec2(0.0f);

		m_Vertices.push_back(vertex);
	}

	// process indices
	for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace face = mesh->mFaces[i];
		for (uint32_t j = 0; j < face.mNumIndices; ++j)
			m_Indices.push_back(face.mIndices[j]);
	}

	// process materials
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		LoadTextures(material, aiTextureType_DIFFUSE);
		LoadTextures(material, aiTextureType_SPECULAR);
	}
}

void Model::LoadTextures(aiMaterial* material, aiTextureType type)
{
	uint32_t textureCount = material->GetTextureCount(type);
	if (textureCount == 0)
	{
		// fallback texture if the texture could not be loaded
		const char* texPath = "assets/textures/checkerboard.png";
		Logger::Warn(" Fallback texture loaded: \"{}\"", texPath);
		m_LoadedTextures.emplace_back(texPath);
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