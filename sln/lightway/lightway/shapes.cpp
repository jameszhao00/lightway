#include "stdafx.h"
#include "shapes.h"
#include "rendering.h"

#include <glm/ext.hpp>
float3 Ray::at(float t) const
{
	return origin + dir * float3(t);
}
void Ray::intersect_with_discs(const Disc* discs, const int num_discs, Intersection* intersections) const
{
	for(int i = 0; i < num_discs; i++)
	{		
		intersections[i].hit = false;
		if(glm::dot(-dir, discs[i].normal) > 0)
		{
			float t = glm::dot(discs[i].center - origin, discs[i].normal) / glm::dot(dir, discs[i].normal);
			if(t > 0)
			{
				float3 pt = dir * t + origin;
				if(glm::length(pt - discs[i].center) < discs[i].radius)
				{
					
					if(glm::dot(discs[i].normal, -dir) > 0)
					{
						intersections[i].hit = true;
						intersections[i].material = discs[i].material;
						intersections[i].position = pt;
						intersections[i].normal = discs[i].normal;
						intersections[i].t = t;
					}
				}
			}
		}
	}
}
void Ray::intersect_with_spheres(const Sphere* spheres, const int num_spheres, Intersection* intersections) const
{
	for(int i = 0; i < num_spheres; i++)
	{
		intersections[i].hit = false;

		const Sphere* sph = &spheres[i];
		float3 l = origin - sph->center;
		float a = 1;//dot(dir,dir);
		float b = 2*glm::dot(dir, l);
		float c = glm::dot(l, l)-sph->radius*sph->radius;

		float discriminant = b*b - 4*a*c;
		
		if(discriminant > 0)
		{
			float t0 = (-b-sqrt(discriminant))/(2*a);
			float t1 = (-b+sqrt(discriminant))/(2*a);
			
			if(t0 > 0 || t1 > 0)
			{
				float t0_clamped = t0 < 0 ? 100000000 : t0;
				float t1_clamped = t1 < 0 ? 100000000 : t1;

				float t = glm::min(t0_clamped, t1_clamped);
				
				float3 hit_pos = t * dir + origin;
				float3 normal = normalize(hit_pos - sph->center);
				if(dot(normal, -dir) > 0)
				{
					intersections[i].hit = true;
					intersections[i].position = hit_pos;
					intersections[i].normal = normal;
					intersections[i].material = sph->material;
					intersections[i].t = t;
				}
			}
		}
	}
}
const float MIN_T = 0.001;
void Ray::intersect_with_triangles(const Triangle* triangles, const int num_tris, Intersection* intersections, bool flip_ray) const
{
	for(int i = 0; i < num_tris; i++)
	{
		intersections[i].hit = false;
		
        if(dot(triangles[i].normal, (flip_ray ? dir: -dir)) > 0) 
        {
			if(!flip_ray)
			{
				float3 barypos;
				bool hit = glm::intersectRayTriangle(origin, dir, 
					triangles[i].vertices[0].position, 
					triangles[i].vertices[1].position, 
					triangles[i].vertices[2].position, 
					barypos);

				if(hit && (barypos.z > MIN_T))
				{
					
					intersections[i].hit = true;
					float3 pos = 
						triangles[i].vertices[0].position * (1-barypos.x-barypos.y) + 
						triangles[i].vertices[1].position * barypos.x + 
						triangles[i].vertices[2].position * barypos.y;
					intersections[i].position = pos;
					intersections[i].material = triangles[i].material;
					intersections[i].t = barypos.z;
					intersections[i].normal = triangles[i].normal;
					
					float3 test = glm::abs((pos - origin) / float3(intersections[i].t) - dir);
					if( test.x > 0.01 || test.y > 0.01 || test.z > 0.01 )
					{
						intersections[i].material = nullptr;
					}
				}
			}
			else
			{
				float3 barypos;
				bool hit = glm::intersectRayTriangle(origin, dir, 
					triangles[i].vertices[2].position, 
					triangles[i].vertices[1].position, 
					triangles[i].vertices[0].position, 
					barypos);

				if(hit && (barypos.z > MIN_T))
				{
					
				intersections[i].hit = true;
					float3 pos = 
						triangles[i].vertices[2].position * (1-barypos.x-barypos.y) + 
						triangles[i].vertices[1].position * barypos.x + 
						triangles[i].vertices[0].position * barypos.y;
					intersections[i].position = pos;
					intersections[i].material = triangles[i].material;
					intersections[i].t = barypos.z;
					intersections[i].normal = triangles[i].normal;
					
				}
			}
            
        }
		if(intersections[i].hit)
		{
			lwassert_greater(dot(intersections[i].normal, (flip_ray ? dir: -dir)), 0);// > 0);
		}
	}
}

bool Ray::intersect_with_aabb(const AABB& aabb, float* t, float* t_max_out) const
{
	//? = min_pt.?, max_pt.?
	float t_mins[] = {-INF, -INF, -INF};
	float t_maxs[] = {INF, INF, INF};
	
	float3 inv_ray_d = float3(1) / (float3(glm::equal(dir, float3(0))) * float3(1) + dir);
	for(int i = 0; i < 3; i++)
	{
		if(dir[i] != 0)
		{
			float t0 = (aabb.min_pt[i] - origin[i]) * inv_ray_d[i];
			float t1 = (aabb.max_pt[i] - origin[i]) * inv_ray_d[i];
			t_mins[i] = glm::min(t0, t1);
			t_maxs[i] = glm::max(t0, t1);
		}
		else
		{
			//if a component has no direction
			//make sure the origin is inside the two bounds
			if(origin[i] >= aabb.min_pt[i] && origin[i] <= aabb.max_pt[i])
			{
				t_mins[i] = -INF;
				t_maxs[i] = INF;
			}
			else
			{
				return false;
			}
		}
	}
	float t_min = glm::max(t_mins[0], t_mins[1], t_mins[2]);
	float t_max = glm::min(t_maxs[0], t_maxs[1], t_maxs[2]);
	*t = glm::max(t_min, 0.f);
	*t_max_out = t_max;
	return t_min < t_max;

} 