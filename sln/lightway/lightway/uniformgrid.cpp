#include "stdafx.h"
#include "uniformgrid.h"
#include "shapes.h"
#include "lwmath.h"
#include "rendering.h"
#include <glm/ext.hpp>


UniformGrid::UniformGrid(int3 p_subdivisions, const AABB& p_aabb)
	: subdivisions(p_subdivisions), 
	aabb(p_aabb),
	cell_dim(p_aabb.extent / float3(p_subdivisions))
{
	
	assert((subdivisions.x > 0) && (subdivisions.y > 0) && (subdivisions.z > 0));
	
	data = new boost::multi_array<Voxel, 3>(boost::extents[subdivisions.x][subdivisions.y][subdivisions.z]);
}
UniformGrid::~UniformGrid()
{
	delete data;
}
const float DDA_T_EPSILON = 0.0001;
bool UniformGrid::dda_vals(const Ray& ray, float3* t_delta, float3* step, float3* t_max, int3* start_cell, int3* outcell) const
{
	float t, t_end;
	bool hit = ray.intersect_with_aabb(aabb, &t, &t_end);
	if(!hit) return false;
	float3 start_pos = ray.at(t);
	//abs this since it should always be positive
	*t_delta = glm::abs(float3(cell_dim/((float3(glm::equal(ray.dir, float3(0))) + ray.dir))));
	//if dir.? == 0 we don't care about it
	*step = float3(
		ray.dir.x > 0 ? 1 : -1, 
		ray.dir.y > 0 ? 1 : -1, 
		ray.dir.z > 0 ? 1 : -1
		);
	//clamp b/c of floating point imprecisions, we can end up with -1 and max+1 cellids
	//its correct here b/c we've definitely hit the aabb
	*start_cell = glm::clamp(cellid_of(start_pos), int3(0, 0, 0), subdivisions - int3(1));
	//find out how much t we have to travel to hit the next voxel edge
	//saturate b/c we don't want the next ll plane if we're going backwards...
	*t_max = float3(
		(ray.dir.x != 0) ? ((lower_left(*start_cell + int3(glm::saturate(step->x), 0, 0)).x - ray.origin.x) / ray.dir.x) : INF,
		(ray.dir.y != 0) ? ((lower_left(*start_cell + int3(0, glm::saturate(step->y), 0)).y - ray.origin.y) / ray.dir.y) : INF,
		(ray.dir.z != 0) ? ((lower_left(*start_cell + int3(0, 0, glm::saturate(step->z))).z - ray.origin.z) / ray.dir.z) : INF);
	//find the termination cellid
	//auto end = ray.at(t_end + glm::min(glm::min(t_delta->x, t_delta->y), t_delta->z) * 0.000000001f);
	//*t_exit = t_end;// + DDA_T_EPSILON;// - DDA_T_EPSILON;// + glm::min(glm::min(t_delta->x, t_delta->y), t_delta->z);
	
	//find the plane which the ray pierces when it exits
	//increment/decrement cellid's value for that axis
	//leave the others untouched
	//essentially we want to find the instantenous outcell
	//not outcell for t_exit + epsilon
	//auto end = ray.at(t_end);
	//we clamp b/c of fp imprecision
	//sometimes we end up outside, sometimes not


	//the below solved the missing triangles issue
	*outcell = int3(
		ray.dir.x > 0 ? subdivisions.x : -1,
		ray.dir.y > 0 ? subdivisions.y : -1,
		ray.dir.z > 0 ? subdivisions.z : -1
		);
	return true;
}
bool UniformGrid::dda_next(const float3& t_delta, const float3& step, const int3& outcell, float3* t_max, int3* cellid, float* debug) const
{	
	float3 target_t_max = *t_max;
	int3 target_cellid = *cellid;
	if(target_t_max.x < target_t_max.y)
	{
		//x or z smallest
		if(target_t_max.x < target_t_max.z)
		{
			if(debug != nullptr) *debug = target_t_max.x;
			//x smallest
			target_cellid.x += step.x;
			if(target_cellid.x == outcell.x) return false;
			target_t_max.x += t_delta.x;
		}
		else
		{
			if(debug != nullptr) *debug = target_t_max.z;
			//z smallest
			target_cellid.z += step.z;
			if(target_cellid.z == outcell.z) return false;
			target_t_max.z += t_delta.z;
		}
	}
	else
	{
		//y or z smallest
		if(target_t_max.y < target_t_max.z)
		{
			if(debug != nullptr) *debug = target_t_max.y;
			//y smallest
			target_cellid.y += step.y;
			if(target_cellid.y == outcell.y) return false;
			target_t_max.y += t_delta.y;
		}
		else
		{
			if(debug != nullptr) *debug = target_t_max.z;
			//z smallest
			target_cellid.z += step.z;
			if(target_cellid.z == outcell.z) return false;
			target_t_max.z += t_delta.z;
		}
	}
	//if(target_t_max.x > t_exit && target_t_max.y > t_exit && target_t_max.z > t_exit) return false;
	*cellid = target_cellid;
	*t_max = target_t_max;
	return true;
}
const float MIN_INTERSECTION_T = 0.000001f;
bool UniformGrid::intersect(const IntersectionQuery& query, Intersection* intersection) const
{
	float3 t_delta, step, t_max;
	int3 cellid;
	int3 outcell;
	bool hit = dda_vals(query.ray, &t_delta, &step, &t_max, &cellid, &outcell);
	if(!hit) return false;
	float dummy;
	do
	{
		//perform intersection test against all elements in cellid
		if(glm::any(glm::lessThan(cellid, int3(0, 0, 0))) || glm::any(glm::greaterThanEqual(cellid, subdivisions)))
		{
			//cout << "cell id = " << cellid << "outcell = " << outcell << " t = " << dummy << endl;
			lwassert(false);
		}
		Voxel* voxel = cell(cellid);
		const Triangle* tris = voxel->data;
		if(tris != nullptr)
		{			
			//we're not using closest_t for comparison
			//because t_max can return close to inf values
			//when we're really close
			bool tri_hit = false;
			Intersection closest_intersection;
			closest_intersection.t = FLT_MAX;
			for(int tri_i = 0; tri_i < voxel->count; tri_i++)
			{
				Intersection tri_intersection;
				query.ray.intersect_with_triangles(&tris[tri_i], 1, &tri_intersection);
				//MIN_INTERSECTION_T is for numerical instabilitiies when we're really close
				if(tri_intersection.hit && tri_intersection.t < closest_intersection.t &&
					query.isValid(tri_intersection))
				{
					
		//lwassert_notequal(tri_intersection.normal, float3(0));
					tri_hit = true;
					closest_intersection = tri_intersection;
					//lwassert_notequal(tri_intersection.normal, float3(0));
				}
			}
			
			//min(tmax.x, .y, .z) = prevent us from covering tris in the next voxels
			//without the epsilon there, if a tri lies on a voxel boundary, it may or may not intersect ever
			float voxel_max_t = glm::min(t_max.x, t_max.y, t_max.z) + 0.0001;

			//no hit -> closest_t = INF
			//this code fails if voxel_max_t is infinitely large...
			if(tri_hit && closest_intersection.t < voxel_max_t)
			{				
				//assert(voxel_max_t > ibuffer[intersection_id].t);
				*intersection = closest_intersection;
		//lwassert_notequal(closest_intersection.normal, float3(0));
				return true;
			}
		}
	} while (dda_next(t_delta, step, outcell, &t_max, &cellid, &dummy));
	
	return false;
}
Voxel* UniformGrid::cell(int3 cellid) const
{
	return &(*data)[cellid.x][cellid.y][cellid.z];
}
void UniformGrid::cell_bound(const int3& cellid, float3* cell_min_pt, float3* cell_max_pt) const
{
	//assert(cellid.x < subdivisions.x && cellid.y < subdivisions.y && cellid.z < subdivisions.z);
	*cell_min_pt = float3(cellid) / float3(subdivisions) * aabb.extent + aabb.min_pt;
	*cell_max_pt = (float3(cellid) + float3(1, 1, 1)) / float3(subdivisions) * aabb.extent + aabb.min_pt;
}
AABB UniformGrid::cell_bound(const int3& cellid) const
{
	float3 cell_min_pt, cell_max_pt;
	cell_bound(cellid, &cell_min_pt, &cell_max_pt);
	return AABB(cell_min_pt, cell_max_pt);
}
int3 UniformGrid::cellid_of(const float3& coord) const
{
	return int3(glm::floor((coord-aabb.min_pt)/aabb.extent * float3(subdivisions)));
}
#include "tribox.h"
unique_ptr<UniformGrid> make_uniform_grid(const StaticScene& scene, int3 subdivisions)
{
	float3 min_pt(10000000), max_pt(-10000000);
	for(size_t tri_i = 0; tri_i < scene.triangles.size(); tri_i++)
	{		
		const Triangle& tri = scene.triangles[tri_i];
		for(auto vert_i = 0; vert_i < 3; vert_i++)
		{
			float3 pos = tri.vertices[vert_i].position;
			min_pt = glm::min(pos, min_pt);
			max_pt = glm::max(pos, max_pt);
		}
	}
	AABB aabb(min_pt - float3(0.1), max_pt + float3(0.1));
	unique_ptr<UniformGrid> ug(new UniformGrid(subdivisions, aabb));
		
	boost::multi_array<vector<Triangle>, 3> temp(boost::extents[subdivisions.x][subdivisions.y][subdivisions.z]);
	for(int x = 0; x < subdivisions.x; x++)
		for(int y = 0; y < subdivisions.y; y++)
			for(int z = 0; z < subdivisions.z; z++)
				temp[x][y][z] = vector<Triangle>();

	for(size_t tri_i = 0; tri_i < scene.triangles.size(); tri_i++)
	{		
		//Ring tri;
		auto verts = scene.triangles[tri_i].vertices;
		float3 tri_max = float3(
			glm::max(verts[0].position.x, verts[1].position.x, verts[2].position.x),
			glm::max(verts[0].position.y, verts[1].position.y, verts[2].position.y),
			glm::max(verts[0].position.z, verts[1].position.z, verts[2].position.z));
		float3 tri_min = float3(
			glm::min(verts[0].position.x, verts[1].position.x, verts[2].position.x),
			glm::min(verts[0].position.y, verts[1].position.y, verts[2].position.y),
			glm::min(verts[0].position.z, verts[1].position.z, verts[2].position.z));
		
		auto cell_min = glm::clamp(ug->cellid_of(tri_min) - int3(1), int3(0), ug->subdivisions - 1);
		//cell max is INCLUSIVE
		auto cell_max = glm::clamp(ug->cellid_of(tri_max) + int3(1), int3(0), ug->subdivisions - 1);
		
		for(int x = cell_min.x; x < cell_max.x + 1; x++)
		{
			for(int y = cell_min.y; y < cell_max.y + 1; y++)
			{
				for(int z = cell_min.z; z < cell_max.z + 1; z++)
				{
					AABB voxel_aabb = ug->cell_bound(int3(x, y, z));
					voxel_aabb.min_pt -= ug->cell_dim * float3(0.01);
					voxel_aabb.max_pt += ug->cell_dim * float3(0.01);
					if(tri_aabb_test(scene.triangles[tri_i], voxel_aabb))
					{
						temp[x][y][z].push_back(scene.triangles[tri_i]);
					}
				}
			}
		}
	}
	
	for(int x = 0; x < subdivisions.x; x++)
		for(int y = 0; y < subdivisions.y; y++)
			for(int z = 0; z < subdivisions.z; z++)
			{
				ug->cell(int3(x, y, z))->init(temp[x][y][z]);
				//ug->cell(ifloat3(x, y, z))->data = nullptr;
				//ug->cell(ifloat3(x, y, z))->count = 0;
			}

	return ug;
}