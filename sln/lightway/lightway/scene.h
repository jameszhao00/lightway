#pragma once
#include <vector>
#include <memory>
#include "rendering.h"
#include "lwmath.h"
#include "bxdf.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "scene.h"
#include "AreaLight.h"
#include <rtcore.h>

using namespace std;
struct Vertex
{
	Vertex() : position(-10000000), normal(-10000000), uv(-10000000) { }
	float3 position;
	float3 normal;
	float2 uv;
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
	Triangle(const float3& v0, const float3& v1, const float3& v2, const float3& n, 
		const Material* mat = nullptr) : material(mat), normal(n) 
	{
		vertices[0].position = v0;
		vertices[1].position = v1;
		vertices[2].position = v2;
	}
	float3 shadingNormals(const float2& uv) const
	{
		return normalize(vertices[0].normal * (1-uv.x-uv.y) + 
			vertices[1].normal * uv.x +
			vertices[2].normal * uv.y);
	}
	void compute_normal() 
	{ 		
		normal = glm::normalize(vertices[0].normal + vertices[1].normal + vertices[2].normal);
	}
	Vertex vertices[3];
	const Material* material;
	float3 normal;
};
struct StaticScene
{
	StaticScene(){ }
	vector<Triangle> triangles;
	vector<unique_ptr<Material>> materials;
};
struct IntersectionQuery;

struct AccelScene
{
	AccelScene() { }
	AccelScene(const AccelScene& other) 
	{
	}
	embree::Ref<embree::Intersector> intersector;
	RectangularAreaLight lights[1];
	const RectangularAreaLight* light(int lightIdx) const
	{
		return &lights[lightIdx];
	}
	void intersect(const IntersectionQuery& query,
		Intersection* sceneIsect,
		Intersection* lightIsect) const;
	
	const StaticScene* data;
};
std::unique_ptr<AccelScene> makeAccelScene(const StaticScene* scene);