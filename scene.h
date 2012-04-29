#pragma once
#include <vector>
#include <memory>
#include "math.h"
#include "bxdf.h"
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
	StaticScene() { }
	vector<Triangle> triangles;
	vector<unique_ptr<Material>> materials;
};