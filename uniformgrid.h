#pragma once
#include <boost/multi_array.hpp>
#include <boost/array.hpp>
#include "math.h"
#include "scene.h"
#include "shapes.h"
using namespace std;
using namespace glm;
struct Intersection;
struct Ray;
struct Voxel
{
	Voxel() : count(0), data(nullptr)
	{ 
	}
	void init(const vector<Triangle>& content)
	{
		destroy();
		
		if(content.size() > 0)
		{
			count = content.size();
			data = new Triangle[count];
			for(int i = 0; i < count; i++)
			{
				data[i] = content[i];
			}
		}
	}
	~Voxel() 
	{ 
		destroy();		
	}
	void destroy()
	{
		if(data != nullptr)
		{
			delete [] data; 
		}
		data = nullptr;
	}
	int count;
	Triangle* data;
	
	//SHOULD NOT CALL the following
	//Voxel(const Voxel& other) { }
	//Voxel& operator=(const Voxel& other) { }
private:
};
class UniformGrid
{
public:
	UniformGrid(ivec3 subdivisions, const AABB& p_aabb);
	~UniformGrid();
	int intersect(const Ray& ray, Intersection* ibuffer, int ibuffer_size, bool flip_ray, bool verbose = false) const;
	Voxel* cell(ivec3 cellid) const;
	void cell_bound(const ivec3& cellid, vec3* cell_min_pt, vec3* cell_max_pt) const;
	AABB cell_bound(const ivec3& cellid) const;
	ivec3 cellid_of(const vec3& coord) const;
	vec3 lower_left(ivec3 cellid) const
	{
		vec3 result, dummy;
		cell_bound(cellid, &result, &dummy);
		return result;
	}
	const ivec3 subdivisions;
	const AABB aabb;
	const vec3 cell_dim;
	inline bool dda_vals(const Ray& ray, vec3* t_delta, vec3* step, vec3* t_max, ivec3* start_cell, ivec3* outcell) const;
	inline bool dda_next(const vec3& t_delta, const vec3& step, const ivec3& outcell,  vec3* t_max, ivec3* cellid, float* debug = nullptr) const;
	//not stack allocated due to boost bug...
	boost::multi_array<Voxel, 3>* data;
};
unique_ptr<UniformGrid> make_uniform_grid(const StaticScene& scene, ivec3 subdivisions);