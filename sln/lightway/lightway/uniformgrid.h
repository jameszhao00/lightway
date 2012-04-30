#pragma once
#include <boost/multi_array.hpp>
#include "lwmath.h"
#include "scene.h"
#include "shapes.h"
using namespace std;
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
			count = (int)content.size();
			data = new Triangle[(size_t)count];
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
	UniformGrid(int3 subdivisions, const AABB& p_aabb);
	~UniformGrid();
	bool intersect(const Ray& ray, Intersection* intersection, bool flip_ray, bool verbose = false) const;
	Voxel* cell(int3 cellid) const;
	void cell_bound(const int3& cellid, float3* cell_min_pt, float3* cell_max_pt) const;
	AABB cell_bound(const int3& cellid) const;
	int3 cellid_of(const float3& coord) const;
	float3 lower_left(int3 cellid) const
	{
		float3 result, dummy;
		cell_bound(cellid, &result, &dummy);
		return result;
	}

	const int3 subdivisions;
	const AABB aabb;
	const float3 cell_dim;
	inline bool dda_vals(const Ray& ray, float3* t_delta, float3* step, float3* t_max, int3* start_cell, int3* outcell) const;
	inline bool dda_next(const float3& t_delta, const float3& step, const int3& outcell,  float3* t_max, int3* cellid, float* debug = nullptr) const;
	//not stack allocated due to boost bug...
	boost::multi_array<Voxel, 3>* data;
	//shouldn't really be here...
	vector<Material*> materials;
};
unique_ptr<UniformGrid> make_uniform_grid(const StaticScene& scene, int3 subdivisions);