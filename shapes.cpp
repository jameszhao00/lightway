#include "shapes.h"
#include "rendering.h"
void Ray::intersect_with_discs(const Disc* discs, const int num_discs, Intersection* intersections) const
{
	for(int i = 0; i < num_discs; i++)
	{		
		intersections[i].hit = false;
		if(dot(-dir, discs[i].normal) > 0)
		{
			float t = dot(discs[i].center - origin, discs[i].normal) / dot(dir, discs[i].normal);
			if(t > 0)
			{
				vec3 pt = dir * t + origin;
				if(length(pt - discs[i].center) < discs[i].radius)
				{
					
					if(dot(discs[i].normal, -dir) > 0)
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
		vec3 l = origin - sph->center;
		float a = 1;//dot(dir,dir);
		float b = 2*dot(dir, l);
		float c = dot(l, l)-sph->radius*sph->radius;

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
				
				vec3 hit_pos = t * dir + origin;
				vec3 normal = normalize(hit_pos - sph->center);
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
void Ray::intersect_with_triangles(const Triangle* triangles, const int num_tris, Intersection* intersections, bool flip_ray) const
{
	for(int i = 0; i < num_tris; i++)
	{
		intersections[i].hit = false;
		
        if(dot(triangles[i].normal, (flip_ray ? dir: -dir)) > 0) 
        {
			if(!flip_ray)
			{
				vec3 barypos;
				bool hit = glm::intersectRayTriangle(origin, dir, 
					triangles[i].vertices[0].position, 
					triangles[i].vertices[1].position, 
					triangles[i].vertices[2].position, 
					barypos);

				intersections[i].hit = hit;
				if(hit)
				{
					vec3 pos = 
						triangles[i].vertices[0].position * (1-barypos.x-barypos.y) + 
						triangles[i].vertices[1].position * barypos.x + 
						triangles[i].vertices[2].position * barypos.y;
					intersections[i].position = pos;
					intersections[i].material = triangles[i].material;
					intersections[i].t = barypos.z;
					intersections[i].normal = triangles[i].normal;

					assert(dot(-ray_d, triangles[i].normal) > 0);
				}
			}
			else
			{
				vec3 barypos;
				bool hit = glm::intersectRayTriangle(origin, dir, 
					triangles[i].vertices[2].position, 
					triangles[i].vertices[1].position, 
					triangles[i].vertices[0].position, 
					barypos);

				intersections[i].hit = hit;
				if(hit)
				{
					vec3 pos = 
						triangles[i].vertices[2].position * (1-barypos.x-barypos.y) + 
						triangles[i].vertices[1].position * barypos.x + 
						triangles[i].vertices[0].position * barypos.y;
					intersections[i].position = pos;
					intersections[i].material = triangles[i].material;
					intersections[i].t = barypos.z;
					intersections[i].normal = triangles[i].normal;

					assert(dot(-ray_d, triangles[i].normal) > 0);
				}
			}
            
        }
	}
}