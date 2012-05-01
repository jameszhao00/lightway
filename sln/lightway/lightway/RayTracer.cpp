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
void axisConversions(const float3& normal, float3x3* zUpToWorld, float3x3* worldToZUp)
{
	//hopefully we can collapse this..
	if(normal.x == 0 && normal.y == 0 && normal.z == 1)
	{
		//do nothing
	}
	else if(normal.x == 0 && normal.y == 0 && normal.z == -1)	
	{
		float3 rotaxis(0, 1, 0);
		float rotangle = PI;
		*zUpToWorld = float3x3(glm::rotate(glm::degrees(rotangle), rotaxis));
	}
	else
	{					
		float3 rotaxis = cross(float3(0, 0, 1), normal);
		float rotangle = acos(dot(normal, float3(0, 0, 1)));
		*zUpToWorld = float3x3(glm::rotate(glm::degrees(rotangle), rotaxis));
	}
	*worldToZUp = glm::transpose(*zUpToWorld);	
}
void RayTracer::process_samples(const RTScene& scene, Rand& rand, Sample* samples_array, int sample_n, SampleDebugger& sd)
{
	Sample& sample = samples_array[0];
	if(sample.depth > RT_MAX_DEPTH) 
	{
		sample.finished = true; 
		return;
	}
	if(sample.depth > RANDOM_TERMINATION_DEPTH)
	{
		//green = luminosity
		float survival = glm::min(0.5f, sample.throughput.g);
		if(rand.next01() > survival)
		{
			sample.finished = true;
			return;
		}
		sample.throughput /= survival;
	}
	Intersection closest;
	bool hit = closest_intersect_ray_scene(scene, sample.ray, &closest, false);
	if(!hit)
	{			
		sample.radiance += sample.throughput * background;
		sample.finished = true;
		return;
	}
	if(sample.depth == 0)
	{
		float3 emission = sample.throughput * closest.material->emission;
		sample.radiance += emission;
	}
		
	float3x3 zUpToWorld;
	float3x3 worldToZUp;
	axisConversions(closest.normal, &zUpToWorld, &worldToZUp);
		
	float3 wo = worldToZUp * -sample.ray.dir;
	//update radiance with direct light
	{
		float3 lightPos;
		float3 wiDirectWorld;
		float lightPdf;	
		float lightT;
		scene.area_lights[0].sample(rand, closest.position, &lightPos, &wiDirectWorld, &lightPdf, &lightT);
		Ray shadow_ray(closest.position +  wiDirectWorld * 0.001f, wiDirectWorld);
		Intersection dummy;
		bool hit = scene.accl->intersect(shadow_ray, &dummy, true, false);

		bool light_facing = dot(wiDirectWorld, scene.area_lights[0].normal) < 0;
		
		if((!hit || (lightT < dummy.t )) && light_facing)
		{					
			float3 wiDirect = worldToZUp * wiDirectWorld;
			float3 brdfEval = closest.material->fresnelBlend.eval(wiDirect, wo);
			float3 ndotl = float3(saturate(dot(closest.normal, wiDirectWorld) ));
			float3 le = scene.area_lights[0].material->emission;
			
			float3 direct = le * brdfEval * ndotl / lightPdf;
			sample.radiance += sample.throughput * direct;
		}	
	}
	//update ray/throughput with brdf
	{
		float3 weight;			
		float3 wiIndirect;
		closest.material->fresnelBlend.sample(wo, rand, &wiIndirect, &weight);
		float3 wiWorldIndirect = zUpToWorld * wiIndirect;
		sample.throughput *= weight;

		const float requiredMinThroughput = 0.0001f;
		if(glm::any(glm::lessThan(sample.throughput, float3(requiredMinThroughput))))
		{
			sample.finished = true;
		}
		else
		{				
			sample.ray = Ray(closest.position + 0.001f * wiWorldIndirect, wiWorldIndirect);
			sample.depth++;
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