#pragma once
#include <vector>
#include <memory>
#include "rendering.h"
#include "math.h"
#include "bxdf.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <AntTweakBar.h>
using namespace glm;
using namespace std;
struct Vertex
{
	Vertex() : position(-10000000), normal(-10000000), uv(-10000000) { }
	vec3 position;
	vec3 normal;
	vec2 uv;
};
struct Material
{	
	LambertBrdf lambert;
	BlinnPhongBrdf phong;
	Btdf refraction;	
	vec3 emission;
};
struct Triangle
{
	Triangle() : material(nullptr), normal(-10000000) { }
	Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Material* mat = nullptr) : material(mat), normal(-100000000) 
	{
		vertices[0] = v0;
		vertices[1] = v1;
		vertices[2] = v2;
		compute_normal();
	}
	Triangle(const vec3& v0, const vec3& v1, const vec3& v2, const vec3& n, 
		const Material* mat = nullptr) : material(mat), normal(n) 
	{
		vertices[0].position = v0;
		vertices[1].position = v1;
		vertices[2].position = v2;
	}
	void compute_normal() 
	{ 		
		normal = normalize(vertices[0].normal + vertices[1].normal + vertices[2].normal);
	}
	Vertex vertices[3];
	const Material* material;
	vec3 normal;
};
struct StaticScene
{
	StaticScene() : active_camera_idx(-1) { }
	vector<Triangle> triangles;
	vector<unique_ptr<Material>> materials;
	vector<Camera> cameras;
	int active_camera_idx;
	void init_tweaks()
	{	
		const char* mat_names[] = {
			"mat1", "mat2", "mat3", "mat4", "mat5", "mat6", "mat7", 
		};
		auto bar = TwNewBar("Scene");
		TwAddVarRW(bar, "Active Camera", TW_TYPE_INT32, &active_camera_idx, "");
		for(size_t mat_i = 0; mat_i < materials.size(); mat_i++)
		{
			Material* mat = materials[mat_i].get();
			stringstream st;
			st << mat_i;
			auto idx_str = st.str();
			TwAddVarRW(bar, (string("Albedo ") + idx_str).c_str(), TW_TYPE_COLOR3F, value_ptr(mat->lambert.albedo), "");
			TwAddVarRW(bar, (string("Roughness ") + idx_str).c_str(), TW_TYPE_FLOAT, &mat->phong.spec_power, "");
			TwAddVarRW(bar, (string("Specular ") + idx_str).c_str(), TW_TYPE_COLOR3F, value_ptr(mat->phong.f0), "");
		}
		
	}
};