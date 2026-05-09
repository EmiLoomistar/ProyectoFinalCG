#include "Model.h"
#include <unordered_map>


Model::Model()
{
}

void Model::LoadModel(const std::string & fileName)
{
	Assimp::Importer importer;//					Pasa de Polygons y Quads a triangulos, modifica orden para el origen, generar normales si el  objeto no tiene, trata v�rtices iguales como 1 solo
	//const aiScene *scene=importer.ReadFile(fileName,aiProcess_Triangulate |aiProcess_FlipUVs|aiProcess_GenSmoothNormals|aiProcess_JoinIdenticalVertices);
	const aiScene *scene = importer.ReadFile(fileName, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);
	if (!scene)
	{	
		printf("Fall� en cargar el modelo: %s \n", fileName, importer.GetErrorString());
		return;
	}
	LoadNode(scene->mRootNode, scene);
	LoadMaterials(scene);
	}

void Model::ClearModel()
{
	for (unsigned int i = 0; i < MeshList.size(); i++)
	{
		if (MeshList[i])
		{
			delete MeshList[i];
			MeshList[i] = nullptr;

		}
	}

	for (unsigned int i = 0; i < TextureList.size(); i++)
	{
		if (TextureList[i])
		{
			delete TextureList[i];
			TextureList[i] = nullptr;
		}
	}

}

void Model::RenderModel()
{
	for (unsigned int i = 0; i < MeshList.size(); i++)
	{
		unsigned int materialIndex = meshTotex[i];
		if (!materialIndex< TextureList.size()&& TextureList[materialIndex])
		{
			TextureList[materialIndex]->UseTexture();
		}
		MeshList[i]->RenderMesh();

	}


}


Model::~Model()
{
}

void Model::LoadNode(aiNode * node, const aiScene * scene)
{
	for (unsigned int i = 0; i <node->mNumMeshes; i++)
	{
		LoadMesh(scene->mMeshes[node->mMeshes[i]], scene);
	}
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		LoadNode(node->mChildren[i], scene);
	}
}

void Model::LoadMesh(aiMesh * mesh, const aiScene * scene)
{

	std::vector<GLfloat> vertices;
	std::vector<unsigned int> indices;
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		vertices.insert(vertices.end(), { mesh->mVertices[i].x,mesh->mVertices[i].y ,mesh->mVertices[i].z });
		//UV
		if (mesh->mTextureCoords[0])//si tiene coordenadas de texturizado
		{
			vertices.insert(vertices.end(), { mesh->mTextureCoords[0][i].x,mesh->mTextureCoords[0][i].y});
		}
		else
		{
			vertices.insert(vertices.end(), { 0.0f,0.0f });
		}
		//Normals importante, las normales son negativas porque la luz interact�a con ellas de esa forma, c�mo se vio con el dado/cubo
		
		if (mesh->mNormals)
			vertices.insert(vertices.end(), { -mesh->mNormals[i].x, -mesh->mNormals[i].y, -mesh->mNormals[i].z });
		else
			vertices.insert(vertices.end(), { 0.0f, 0.0f, -1.0f });
	}
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	Mesh* newMesh = new Mesh();
	newMesh->CreateMesh(&vertices[0], &indices[0], vertices.size(), indices.size());
	MeshList.push_back(newMesh);
	meshTotex.push_back(mesh->mMaterialIndex);
}

void Model::LoadMaterials(const aiScene * scene)
{
	std::unordered_map<std::string, Texture*> loadedTextures;
	TextureList.resize(scene->mNumMaterials);
	for (unsigned int i = 0; i < scene->mNumMaterials; i++)
	{
		aiMaterial* material = scene->mMaterials[i];
		TextureList[i] = nullptr;
		// Buscar textura: primero DIFFUSE (OBJ/FBX), luego BASE_COLOR (GLB/GLTF2 PBR)
		aiTextureType texType = aiTextureType_NONE;
		if (material->GetTextureCount(aiTextureType_DIFFUSE))
			texType = aiTextureType_DIFFUSE;
		else if (material->GetTextureCount(aiTextureType_BASE_COLOR))
			texType = aiTextureType_BASE_COLOR;

		if (texType != aiTextureType_NONE)
		{
			aiString path;
			if (material->GetTexture(texType, 0, &path) == AI_SUCCESS)
			{
				std::string pathStr = std::string(path.data);

				// --- Textura embebida (GLB): la ruta empieza con '*' ---
				if (pathStr[0] == '*')
				{
					int texIndex = atoi(pathStr.c_str() + 1); // índice numérico tras el '*'
					if (texIndex < (int)scene->mNumTextures)
					{
						aiTexture* embeddedTex = scene->mTextures[texIndex];
						Texture* newTex = new Texture();
						bool loaded = false;
						if (embeddedTex->mHeight == 0)
						{
							// Textura comprimida (jpg/png en memoria)
							loaded = newTex->LoadTextureFromMemory(
								(unsigned char*)embeddedTex->pcData,
								embeddedTex->mWidth
							);
						}
						else
						{
							// Píxeles crudos BGRA (mWidth x mHeight)
							loaded = newTex->LoadTextureFromRawPixels(
								(unsigned char*)embeddedTex->pcData,
								embeddedTex->mWidth,
								embeddedTex->mHeight
							);
						}
						if (loaded)
						{
							TextureList[i] = newTex;
						}
						else
						{
							printf("Fallo al cargar textura embebida índice %d\n", texIndex);
							delete newTex;
						}
					}
				}
				// --- Textura externa (FBX/OBJ): buscar en Textures/ ---
				else
				{
					int idx;
					std::string filename;
					if (pathStr.rfind("\\") != std::string::npos)
					{
						idx = pathStr.rfind("\\");
						filename = pathStr.substr(idx + 1);
					}
					else if (pathStr.rfind("/") != std::string::npos)
					{
						idx = pathStr.rfind("/");
						filename = pathStr.substr(idx + 1);
					}
					else
					{
						filename = pathStr;
					}

					std::string texPath = std::string("Textures/") + filename;
					auto it = loadedTextures.find(texPath);
					if (it != loadedTextures.end())
					{
						TextureList[i] = it->second;
					}
					else
					{
						Texture* newTex = new Texture(texPath.c_str());
						std::string ext = filename.substr(filename.find_last_of('.') + 1);
						bool loaded = false;
						if (ext == "tga" || ext == "png")
							loaded = newTex->LoadTextureA();
						else
							loaded = newTex->LoadTexture();

						if (loaded)
						{
							TextureList[i] = newTex;
							loadedTextures[texPath] = newTex;
						}
						else
						{
							printf("Falló en cargar la Textura: %s\n", texPath.c_str());
							delete newTex;
						}
					}
				}
			}
		}
		// Textura por defecto si no se pudo cargar ninguna
		if (!TextureList[i])
		{
			TextureList[i] = new Texture("Textures/plain.png");
			TextureList[i]->LoadTextureA();
		}
	}
}
