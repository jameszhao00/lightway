#include "stdafx.h"
#include "asset.h"
#include "lwmath.h"
#include "rendering.h"
#include <glm/ext.hpp>
#include <assimp/assimp.hpp>  
#include <assimp/aiScene.h>   
#include <assimp/aiPostProcess.h> 
using namespace Assimp;


float3 to_glm(const aiVector3D& v)
{
	return float3(v.x, v.y, v.z);
}
float2 to_glm(const aiVector2D& v)
{
	return float2(v.x, v.y);
}
unique_ptr<StaticScene> load_scene(string path, float3 translation, float scale)
{
	unique_ptr<StaticScene> scene(new StaticScene());
	Assimp::Importer importer;
	const aiScene* ai_scene = importer.ReadFile( path, 
		aiProcess_PreTransformVertices	 |
        aiProcess_Triangulate            |
		aiProcess_GenNormals			|
		aiProcess_FixInfacingNormals | 
        aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType);

	float4x4 t = glm::translate(translation) * glm::scale(float3(scale));

	lwassert(ai_scene);
	for(unsigned int i = 0; i < ai_scene->mNumMeshes; i++)
	{
		auto ai_mesh = ai_scene->mMeshes[i];
		int faces_n = ai_mesh->mNumFaces;
		unique_ptr<Material> material(new Material());
		{
			aiColor3D spec;
			aiColor3D diffuse;
			ai_scene->mMaterials[ai_mesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
			ai_scene->mMaterials[ai_mesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_SPECULAR, spec);
			aiString matName;
			ai_scene->mMaterials[ai_mesh->mMaterialIndex]->Get(AI_MATKEY_NAME, matName);
			
			if(string(matName.data).find("refract") != string::npos)
			{
				material->type = Material::SpecularTransmission;
				material->specularTransmission.color = float3(spec.r, spec.g, spec.b);
				material->specularTransmission.iorOutside = 1;
				material->specularTransmission.iorInside = 1.2;
			}
			else if(diffuse.IsBlack() && !spec.IsBlack())
			{
				material->type = Material::SpecularReflection;
				material->specularReflection.specColor = float3(spec.r, spec.g, spec.b);
			}
			else if(spec.IsBlack() && !diffuse.IsBlack())
			{
				material->type = Material::Diffuse;
				material->diffuse.albedo = float3(diffuse.r, diffuse.g, diffuse.b);
			}
		}
		//material->refraction.enabled = false;
		for(int faces_i = 0; faces_i < faces_n; faces_i++)
		{
			Triangle triangle;
			auto face = ai_mesh->mFaces[faces_i];
			for(int vert_i = 0; vert_i < 3; vert_i++)
			{				
				auto index = face.mIndices[vert_i];
				triangle.vertices[vert_i].position = float3(t * float4(to_glm(ai_mesh->mVertices[index]), 1));
				triangle.vertices[vert_i].normal = to_glm(ai_mesh->mNormals[index]);
				triangle.vertices[vert_i].uv = float2(0);
			}
			triangle.material = material.get();
			triangle.compute_normal();
			scene->triangles.push_back(triangle);
		}
		
		scene->materials.push_back(move(material));
	}
	return scene;
}