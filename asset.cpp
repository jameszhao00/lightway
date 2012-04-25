#include "asset.h"
#include <glm/glm.hpp>
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
        //aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType);

	mat4 t = glm::translate(translation) * glm::scale(vec3(scale));
	
	auto error = importer.GetErrorString();
	assert(ai_scene);
	for(int i = 0; i < ai_scene->mNumMeshes; i++)
	{
		auto ai_mesh = ai_scene->mMeshes[i];
		int faces_n = ai_mesh->mNumFaces;
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

			triangle.compute_normal();
			scene->triangles.push_back(triangle);
		}
	}
	return scene;
}