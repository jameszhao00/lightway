#include "pch.h"
#include "asset.h"
#include "math.h"
#include "rendering.h"
#include <glm/ext.hpp>
#include <assimp/assimp.hpp>  
#include <assimp/aiScene.h>   
#include <assimp/aiPostProcess.h> 
using namespace Assimp;


vec3 to_glm(const aiVector3D& v)
{
	return vec3(v.x, v.y, v.z);
}
vec2 to_glm(const aiVector2D& v)
{
	return vec2(v.x, v.y);
}
unique_ptr<StaticScene> load_scene(string path, vec3 translation, float scale)
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

	mat4 t = glm::translate(translation) * glm::scale(vec3(scale));
	if(ai_scene->HasCameras())
	{
		for(int i = 0; i < ai_scene->mNumCameras; i++)
		{
			auto ai_camera = ai_scene->mCameras[i];
			Camera camera;
			camera.eye = to_glm(ai_camera->mPosition);
			camera.ar = ai_camera->mAspect;
			camera.forward = normalize(to_glm(ai_camera->mLookAt));
			camera.fovy = ai_camera->mHorizontalFOV;
			camera.up = to_glm(ai_camera->mUp);
			camera.zf = ai_camera->mClipPlaneFar;
			camera.zn = ai_camera->mClipPlaneNear;
			scene->cameras.push_back(camera);
		}
	}
	auto error = importer.GetErrorString();
	lwassert(ai_scene);
	for(int i = 0; i < ai_scene->mNumMeshes; i++)
	{
		auto ai_mesh = ai_scene->mMeshes[i];
		int faces_n = ai_mesh->mNumFaces;
		unique_ptr<Material> material(new Material());
		{
			aiColor3D diffuse;
			ai_scene->mMaterials[ai_mesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
			material->lambert.albedo = vec3(diffuse.r, diffuse.g, diffuse.b);
			if(glm::any(glm::lessThan(material->lambert.albedo, vec3(0))))
			{
				cout << "clamping albedo to 0" << endl;
				material->lambert.albedo = glm::clamp(material->lambert.albedo, vec3(0), vec3(FLT_MAX));
			}
		}
		material->phong.f0 = vec3(.1);
		material->phong.spec_power = 10;
		material->refraction.enabled = false;
		for(int faces_i = 0; faces_i < faces_n; faces_i++)
		{
			Triangle triangle;
			auto face = ai_mesh->mFaces[faces_i];
			for(int vert_i = 0; vert_i < 3; vert_i++)
			{				
				auto index = face.mIndices[vert_i];
				triangle.vertices[vert_i].position = vec3(t * vec4(to_glm(ai_mesh->mVertices[index]), 1));
				triangle.vertices[vert_i].normal = to_glm(ai_mesh->mNormals[index]);
				triangle.vertices[vert_i].uv = vec2(0);
			}
			triangle.material = material.get();
			triangle.compute_normal();
			scene->triangles.push_back(triangle);
		}
		
		scene->materials.push_back(move(material));
	}
	return scene;
}