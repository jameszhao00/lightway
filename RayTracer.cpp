#include "pch.h"
#include "RayTracer.h"
#include "debug.h"
#include "sampling.h"
#include "bxdf.h"
#include <glm/ext.hpp>
#include <iostream>
//returns -1 if doesn't intersect
/*
int closest_intersect_ray_scene(const RTScene& scene, const Ray& ray, Intersection* intersection_buffer, int buffer_size, bool flip_triangle_dir)
{
	
	int num_intersections_used = 0;
	ray.intersect_with_spheres(scene.spheres, num_spheres, &intersection_buffer[num_intersections_used]);
	num_intersections_used += num_spheres;
	ray.intersect_with_discs(scene.discs, num_discs, &intersection_buffer[num_intersections_used]);
	num_intersections_used += num_discs;
	ray.intersect_with_triangles(scene.triangles.data(), scene.triangles.size(), &intersection_buffer[num_intersections_used], flip_triangle_dir);	
	num_intersections_used += scene.triangles.size();

	ray.intersect_with_triangles(scene.scene->triangles.data(), scene.scene->triangles.size(), 
		&intersection_buffer[num_intersections_used], flip_triangle_dir);
	num_intersections_used += scene.scene->triangles.size();

	assert(num_intersections_used < buffer_size);
    return closest_intersection(intersection_buffer, num_intersections_used);
	
	int hit_i = scene.accl->intersect(ray, intersection_buffer, buffer_size, flip_triangle_dir);
	return hit_i;
}*/
const int RANDOM_TERMINATION_DEPTH = 2;
const int MAX_DEPTH = 4;

void RTScene::init_tweaks()
{		 
	bar = TwNewBar("Material");
	TwAddVarRW(bar, "Ball Albedo", TW_TYPE_COLOR3F, value_ptr(materials[0].lambert.albedo), "");
	TwAddVarRW(bar, "Ball Roughness", TW_TYPE_FLOAT, &materials[0].phong.spec_power, "");
	TwAddVarRW(bar, "Ball Specular", TW_TYPE_COLOR3F, value_ptr(materials[0].phong.f0), "");
		
	TwAddVarRW(bar, "Ball2 Albedo", TW_TYPE_COLOR3F, value_ptr(materials[2].lambert.albedo), "");
	TwAddVarRW(bar, "Ball2 Roughness", TW_TYPE_FLOAT, &materials[2].phong.spec_power, "");
	TwAddVarRW(bar, "Ball2 Specular", TW_TYPE_COLOR3F, value_ptr(materials[2].phong.f0), "");
}
void RayTracer::process_samples(const RTScene& scene, Rand& rand, Sample* samples_array, int sample_n, bool debug)
{
	for(int sample_i = 0; sample_i < sample_n; sample_i++)
	{
		Sample* sample = &samples_array[sample_i];
		if(sample->depth > MAX_DEPTH) 
		{
			sample->finished = true;
			continue;
		}
		
		if(sample->depth > RANDOM_TERMINATION_DEPTH)
		{
			//green = luminosity
			float survival = glm::min(0.5f, sample->throughput.g);
			if(rand.next01() > survival)
			{
				sample->finished = true;
				continue;
			}
			//make up for the path termination
			sample->throughput /= survival;
			//not correct... but okay
			//sample->throughput = saturate(sample->throughput);
		}
		//trace the ray and find an intersection
		Intersection closest;
		closest.normal = vec3(-100, 100, 100);
		bool hit = scene.accl->intersect(sample->ray, &closest, false, debug);
			//closest_intersect_ray_scene(scene, sample->ray, i_buffer, ibuffer_size, false);
		if(!hit)
		{
			sample->radiance += sample->throughput * background;
			sample->finished = true;
			continue;
		}
		
		//Intersection closest = i_buffer[closest_i];
		/*
		if(dot(-sample->ray.dir, closest.normal) <= 0)
		{
			closest_intersect_ray_scene(scene, sample->ray, i_buffer, ibuffer_size, false);
		}
		*/
		//update radiance with direct light
		{
			vec3 light_dir = normalize(scene.area_lights[0].sample_pt(rand) - closest.position);
			Ray shadow_ray(closest.position +  light_dir * 0.0001f, light_dir);
			Intersection dummy;
			bool hit =  scene.accl->intersect(shadow_ray, &dummy, true, false);
				//closest_intersect_ray_scene(scene, shadow_ray, i_buffer, ibuffer_size, true);
			if(!closest.material->refraction.enabled)
			{
				bool visibility = !hit;
				lwassert_validvec(-sample->ray.dir);
				vec3 direct = (closest.material->lambert.eval() + closest.material->phong.eval(-light_dir, -sample->ray.dir)) * 
					saturate(dot(closest.normal, light_dir)) * vec3(visibility) * scene.area_lights[0].color;
				direct /= scene.area_lights[0].pdf();
				sample->radiance += sample->throughput * direct;
			}
		}
		//update ray/throughput with brdf
		{
			mat4 rot;
			//indirect lighting
			//if(closest.normal.x != 0 || closest.normal.y != 0 || closest.normal.z != 1)
			{
				//hopefully we can collapse this..
				if(closest.normal.x == 0 && closest.normal.y == 0 && closest.normal.z == 1)
				{
					//do nothing
				}
				else if(closest.normal.x == 0 && closest.normal.y == 0 && closest.normal.z == -1)	
				{
					vec3 rotaxis(0, 1, 0);
					float rotangle = PI;
					rot = rotate(degrees(rotangle), rotaxis);
				}
				else
				{					
					vec3 rotaxis = cross(vec3(0, 0, 1), closest.normal);
					float rotangle = acos(dot(closest.normal, vec3(0, 0, 1)));
					rot = rotate(degrees(rotangle), rotaxis);
				}				
				
				lwassert_notequal(closest.normal, vec3(0));
				LWASSERT_VALIDVEC(vec3(rot[0]));
				LWASSERT_VALIDVEC(vec3(rot[1]));
				LWASSERT_VALIDVEC(vec3(rot[2]));
				LWASSERT_VALIDVEC(vec3(rot[3]));
			}
			vec3 wi_world;
			vec3 weight;
			bool sample_spec = rand.next01() > 0.5;
			auto ndotv = dot(-sample->ray.dir, closest.normal);
			if(sample_spec)
			{
				vec3 wo_local = vec3(transpose(rot) * vec4(-sample->ray.dir, 0));
				LWASSERT_VALIDVEC(-sample->ray.dir);
				LWASSERT_VALIDVEC(wo_local);
				vec3 wi_local;
				lwassert_greater(wo_local.z, 0);
				closest.material->phong.sample(wo_local, rand, &wi_local, &weight);
				lwassert_allge(weight, vec3(0));
				lwassert(wi_world.length() > 0 || weight.length() == 0);
				wi_world = vec3(rot * vec4(wi_local, 0));
			}
			else
			{
				vec3 wi_local;
				closest.material->lambert.sample(vec2(rand.next01(), rand.next01()), &wi_local, &weight);	
				lwassert_allge(weight, vec3(0));			
				wi_world = vec3(rot * vec4(wi_local, 0));		
			}
		
			sample->throughput *= weight;
			if(sample->throughput.x == 0 && sample->throughput.y == 0 && sample->throughput.z == 0)
			{
				sample->finished = true;
			}
			else
			{				
				Ray wi_ray = Ray(closest.position + 0.0001f * wi_world, wi_world);
				sample->ray = wi_ray;
				sample->depth++;
			}
		}
	}
}
int RayTracer::raytrace(DebugDraw& dd, int total_groups, int my_group, int groups_n, bool clear_fb)
{
	bool flip = false;
	float zn = 1; float zf = 50;
	int rays_pp = 1;
	mat4 inv_view = inverse(camera.view());
	mat4 inv_proj = inverse(camera.projection());
	//Intersection intersections[2];
	vec3 o = camera.eye;
	//const int ibuffer_size = 6000;
	//Intersection i_buffer[ibuffer_size];
	//for(int i = 0; i < num_spheres; i++) dd.add_sphere(scene.spheres[i], vec3(.1, 0, 0));
	//dd.add_disc(scene.discs[0], vec3(0, 1, 0));
	
    int i_start = (int)floor((float)my_group/total_groups * h);
    int i_end = (int)floor((float)(my_group+1)/total_groups * h);
	if(my_group == 0) 
	{
		//for(size_t i = 0; i < scene.scene->triangles.size(); i++) dd.add_tri(scene.scene->triangles[i], vec3(0, 1, 0));
	}
    
    //printf("thread %d processing from %d to %d\n", my_group, i_start, i_end);
    int samples_n = 0;

	for(int i = 0; i < h; i++)
	{
		if((i % groups_n) != my_group) continue;
		for(int j = 0; j < w; j++)
		{
            bool debug = false;
            int fb_i = i * w + j;
            if(j == debug_pixel.x && i == debug_pixel.y)
			{
				debug = true;
			}
			Sample* sample = &sample_fb[fb_i];
			
			
			
			if(sample->finished)
			{				
				
				samples_n += 1;
				vec3 color = sample->radiance;

				vec3 existing_color = vec3(linear_fb[fb_i]);
				float samples_count = linear_fb[fb_i].w + 1;

				vec3 mixed_color = vec3((samples_count - 1) / samples_count) * existing_color + 
					vec3(1/samples_count) * color;
				linear_fb[fb_i] = vec4(mixed_color, samples_count);
            
				vec3 tonemapped = mixed_color / (vec3(1) + mixed_color);
				vec3 srgb = pow(tonemapped, vec3(1/2.2f));
			
				fb[fb_i].r = (int)floor(srgb.x * 255);
				fb[fb_i].g = (int)floor(srgb.y * 255);
				fb[fb_i].b = (int)floor(srgb.z * 255);
				fb[fb_i].a = 255;
			}
			if(sample->finished || clear_fb)
			{
				sample->reset();

				if(clear_fb)
				{
					linear_fb[fb_i] = vec4(0);
				}
				
				vec2 pixel_pos(j, i);			
				vec2 sample_pos = pixel_pos + vec2(rand[my_group].next01(), rand[my_group].next01());					
				vec4 ndc((sample_pos.x/w-0.5)*2, (sample_pos.y/h-0.5)*2, 1, 1);
				vec4 d_comp = inv_proj * ndc;
				d_comp = d_comp/d_comp.w;
				d_comp = inv_view * d_comp;
				vec3 d = normalize(vec3(d_comp));
				Ray ray(o, d);

				sample->ray = ray;
			}
			if(0)//debug)
			{
				if(sample->depth == 0)
				{
					dd.flip();
					//test full traversal
					Ray ray = sample->ray;//Ray ray(vec3(0, 1,5), normalize(vec3(-0.20645134,-0.14003338,-0.96838450 )));
					dd.add_ray(ray);
					vec3 t_delta, step, t_max;
					ivec3 cellid;
					ivec3 out_cell;
					
					scene.accl->dda_vals(ray, &t_delta, &step, &t_max, &cellid, &out_cell);
					//dd.add_sphere(Sphere(ray.at(t_exit), 0.005f, nullptr), vec3(1, 0, 0));
					for(int i = 0; i < 8; i++)
					{			
						{
							auto voxel = scene.accl->cell(cellid);
							const Triangle* tris = voxel->data;
							for(int i = 0; i < voxel->count; i++)
							{
								dd.add_tri(tris[i], vec3(0, 1, 0));
								//cout << "hit tri with ndotv = " << dot(-sample->ray.dir, tris[i].normal) << endl;
							}
						}
						float debugVal = -1;
						bool hit = scene.accl->dda_next(t_delta, step, out_cell, &t_max, &cellid, &debugVal);
						if(!hit) break;
						//cout << "iteration t " << debugVal << endl;
						if(glm::any(glm::lessThan(cellid, ivec3(0))) || glm::any(glm::greaterThanEqual(cellid, scene.accl->subdivisions)))
						{
							dd.add_sphere(Sphere(ray.at(debugVal), 0.01f, nullptr), vec3(1, 0, 1));
						}
						else
						{
							dd.add_sphere(Sphere(ray.at(debugVal), 0.01f, nullptr), vec3(0, 0, 1));
						
						}
						//if(i == 1)
						
					}
					for(int i = 0; i < scene.accl->subdivisions.x; i++)
						for(int j = 0; j < scene.accl->subdivisions.y; j++)
							for(int k = 0; k < scene.accl->subdivisions.z; k++)
							{
							
								dd.add_aabb(scene.accl->cell_bound(ivec3(i, j, k)));
							}
					
				}
			}
			//while(!sample->finished)
			{
				process_samples(scene, rand[my_group], sample, 1, debug && sample->depth == 0);
			}
				
			
		}
	}
	dd.flip();
    return samples_n;
}

void RayTracer::resize(int w, int h)
{
    this->w = w; this->h = h;
}