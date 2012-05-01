#include "stdafx.h"
#include "RayTracer.h"
#include "debug.h"
#include "sampling.h"
#include "bxdf.h"
#include <glm/ext.hpp>
#include <iostream>
//returns -1 if doesn't intersect

bool closest_intersect_ray_scene(const RTScene& scene, const Ray& ray, Intersection* intersection, bool flip_ray)
{
	Intersection scene_intersection;
	bool scene_hit = scene.accl->intersect(ray, &scene_intersection, flip_ray, false);

	Intersection light_intersection;
	bool light_hit = scene.area_lights[0].intersect(ray, &light_intersection, flip_ray);
	if(scene_hit && light_hit)
	{
		*intersection = scene_intersection.t < light_intersection.t ? scene_intersection : light_intersection;
		return true;
	}
	else if(scene_hit)
	{
		*intersection = scene_intersection;
		return true;
	}
	else if(light_hit)
	{
		*intersection = light_intersection;
		return true;
	}
	return false;
}
const int RANDOM_TERMINATION_DEPTH = 2;
const int RT_MAX_DEPTH = 8;

void RayTracer::process_samples(const RTScene& scene, Rand& rand, Sample* samples_array, int sample_n, SampleDebugger& sd)
{
	for(int sample_i = 0; sample_i < sample_n; sample_i++)
	{
		Sample* sample = &samples_array[sample_i];
		if(sample->depth > RT_MAX_DEPTH) 
		{
			sample->finished = true;
			continue;
		}
		sd.shr.record(sample->xy, sample->depth, "T", sample->throughput);
		//russian roulette
		if(sample->depth > RANDOM_TERMINATION_DEPTH)
		{
			//green = luminosity
			float survival = glm::min(0.5f, sample->throughput.g);
			if(rand.next01() > survival)
			{
				sample->finished = true;
				continue;
			}
			sample->throughput /= survival;
		}
		sd.shr.record(sample->xy, sample->depth, "T (RR)", sample->throughput);
		//trace the ray and find an intersection
		Intersection closest;
		closest.normal = float3(-100, 100, 100);		
		bool hit = closest_intersect_ray_scene(scene, sample->ray, &closest, false);
		if(!hit)
		{			
			sample->radiance += sample->throughput * background;
			sample->finished = true;
			sd.shr.record(sample->xy, sample->depth, "Missed All Rad", sample->radiance);
			sd.shr.record(sample->xy, sample->depth, "Missed All", float3(1));
			continue;
		}
		sd.shr.record(sample->xy, sample->depth, "Hit Normal", closest.normal);
		sd.shr.record(sample->xy, sample->depth, "Hit Pos", closest.position);
		sd.shr.record(sample->xy, sample->depth, "Hit t", float3(closest.t));
		if(sample->depth == 0 || sample->specular_using_brdf)
		{
			sd.shr.record(sample->xy, sample->depth, "SpecEmission", sample->throughput * closest.material->emission);
			sample->radiance += sample->throughput * closest.material->emission;
			sample->specular_using_brdf = false;
		}

		//update radiance with direct light
		{
			//diffuse - sample using light pdf
			{			
				float3 light_pt = scene.area_lights[0].sample_pt(rand);
				sd.shr.record(sample->xy, sample->depth, "LightPos", light_pt);
				
				float3 light_dir = normalize(light_pt - closest.position);
				float light_t = (light_pt.x - closest.position.x) / light_dir.x;
				Ray shadow_ray(closest.position +  light_dir * 0.001f, light_dir);
				Intersection dummy;
				bool hit = scene.accl->intersect(shadow_ray, &dummy, true, false);

				bool light_facing = dot(light_dir, scene.area_lights[0].normal) < 0;
				
				if((!hit || (light_t < dummy.t )) && light_facing)
				{
					lwassert_validvec(-sample->ray.dir);
					//visibility is implied
					float3 phong = float3(0);//closest.material->phong.eval(-light_dir, -sample->ray.dir);
					float3 lambert = closest.material->lambert.eval();
					float3 sndotl = float3(saturate(dot(closest.normal, light_dir) ));
					sd.shr.record(sample->xy, sample->depth, "DLight sndotl", sndotl);
					//lndotl remembered from radiosity hw
					float3 lndotl = float3(saturate(dot(scene.area_lights[0].normal, -light_dir)));
					sd.shr.record(sample->xy, sample->depth, "DLight lndotl", lndotl);
					float3 direct = (lambert + phong) * lndotl * sndotl * scene.area_lights[0].material->emission;
					direct /= scene.area_lights[0].pdf();

					sample->radiance += sample->throughput * direct;
					sd.shr.record(sample->xy, sample->depth, "DLight Rad", sample->throughput * direct);
				}
					
			}
		}
		//update ray/throughput with brdf
		{
			float4x4 rot;

			{
				//hopefully we can collapse this..
				if(closest.normal.x == 0 && closest.normal.y == 0 && closest.normal.z == 1)
				{
					//do nothing
				}
				else if(closest.normal.x == 0 && closest.normal.y == 0 && closest.normal.z == -1)	
				{
					float3 rotaxis(0, 1, 0);
					float rotangle = PI;
					rot = glm::rotate(glm::degrees(rotangle), rotaxis);
				}
				else
				{					
					float3 rotaxis = cross(float3(0, 0, 1), closest.normal);
					float rotangle = acos(dot(closest.normal, float3(0, 0, 1)));
					rot = glm::rotate(glm::degrees(rotangle), rotaxis);
				}				
				
				lwassert_notequal(closest.normal, float3(0));
				LWASSERT_VALIDVEC(float3(rot[0]));
				LWASSERT_VALIDVEC(float3(rot[1]));
				LWASSERT_VALIDVEC(float3(rot[2]));
				LWASSERT_VALIDVEC(float3(rot[3]));
			}
			float3 wi_world;
			float3 weight;
			bool sample_spec = rand.next01() > 0.5;
			auto ndotv = dot(-sample->ray.dir, closest.normal);
			if(sample_spec)
			{
				float3 wo_local = float3(glm::transpose(rot) * float4(-sample->ray.dir, 0));
				
				sd.shr.record(sample->xy, sample->depth, "Spec wo Local", wo_local);
				LWASSERT_VALIDVEC(-sample->ray.dir);
				LWASSERT_VALIDVEC(wo_local);
				float3 wi_local;
				lwassert_greater(wo_local.z, 0);
				do {
				closest.material->phong.sample(wo_local, rand, &wi_local, &weight);
				} while(wi_local.z < 0);
				sd.shr.record(sample->xy, sample->depth, "Spec wi Local", wi_local);
				lwassert_allge(weight, float3(0));
				lwassert(wi_world.length() > 0 || weight.length() == 0);
				wi_world = float3(rot * float4(wi_local, 0));
				sd.shr.record(sample->xy, sample->depth, "Spec wi world", wi_world);
				sd.shr.record(sample->xy, sample->depth, "Spec weight", weight);
				sample->specular_using_brdf = true;
			}
			else
			{
				float3 wi_local;
				closest.material->lambert.sample(float2(rand.next01(), rand.next01()), &wi_local, &weight);	
				lwassert_allge(weight, float3(0));			
				wi_world = float3(rot * float4(wi_local, 0));
				sd.shr.record(sample->xy, sample->depth, "Diffuse wi local", wi_local);
				sd.shr.record(sample->xy, sample->depth, "Diffuse wi world", wi_world);
				sd.shr.record(sample->xy, sample->depth, "Diffuse weight", weight);
			}
		
			sample->throughput *= weight;
			const float throughput_epsilon = 0.0001f;
			if(sample->throughput.x < throughput_epsilon
				&& sample->throughput.y < throughput_epsilon
				&& sample->throughput.z < throughput_epsilon)
			{
				sample->finished = true;
			}
			else
			{				
				Ray wi_ray = Ray(closest.position + 0.001f * wi_world, wi_world);
				sample->ray = wi_ray;
				sample->depth++;
			}
			
			sd.shr.record(sample->xy, sample->depth, "Radiance", sample->radiance);
		}
	}
}
int RayTracer::raytrace(int total_groups, int my_group, int groups_n, bool clear_fb, SampleDebugger& sd)
{
	bool flip = false;
	float zn = 1; float zf = 50;
	int rays_pp = 1;
	const Camera* active_camera = &camera;
	if(scene->scene->active_camera_idx != -1)
	{
		active_camera = &scene->scene->cameras[scene->scene->active_camera_idx];
	}
	float4x4 inv_view = glm::inverse(active_camera->view());
	float4x4 inv_proj = glm::inverse(active_camera->projection());
	float3 o = active_camera->eye;
		
    int i_start = (int)floor((float)my_group/total_groups * h);
    int i_end = (int)floor((float)(my_group+1)/total_groups * h);

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
				float3 color = sample->radiance;
			
				float3 existing_color = float3(linear_fb[fb_i]);
				sd.shr.record(sample->xy, 7, "Finish Existing Color", existing_color);
				float samples_count = linear_fb[fb_i].w + 1;

				float3 mixed_color = float3((samples_count - 1) / samples_count) * existing_color + 
					float3(1/samples_count) * color;
				sd.shr.record(sample->xy, 7, "Finish Mix Color", mixed_color);
				linear_fb[fb_i] = float4(mixed_color, samples_count);
				
			}
			if(sample->finished || clear_fb)
			{
				sample->reset();

				if(clear_fb)
				{
					linear_fb[fb_i] = float4(0);
				}
				
				float2 pixel_pos(j, i);			
				float2 sample_pos = pixel_pos + float2(rand[my_group].next01(), rand[my_group].next01());					
				float4 ndc((sample_pos.x/w-0.5)*2, (sample_pos.y/h-0.5)*2, 1, 1);
				float4 d_comp = inv_proj * ndc;
				d_comp = d_comp/d_comp.w;
				d_comp = inv_view * d_comp;
				float3 d = normalize(float3(d_comp));
				Ray ray(o, d);

				sample->ray = ray;
				sample->xy = int2(j, i);
				sd.shr.newSample(sample->xy);
			}
			process_samples(*scene, rand[my_group], sample, 1, sd);
			
				
			
		}
	}
    return samples_n;
}

void RayTracer::resize(int w, int h)
{
    this->w = w; this->h = h;
}