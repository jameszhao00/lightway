#include "RayTracer.h"
#include "debug.h"
#include "sampling.h"
#include "bxdf.h"
#include <iostream>
//returns -1 if doesn't intersect
int closest_intersect_ray_scene(const Scene& scene, const Ray& ray, Intersection* intersection_buffer, int buffer_size, bool flip_triangle_dir)
{
	int num_intersections_used = 0;
	ray.intersect_with_spheres(scene.spheres, num_spheres, &intersection_buffer[num_intersections_used]);
	num_intersections_used += num_spheres;
	ray.intersect_with_discs(scene.discs, num_discs, &intersection_buffer[num_intersections_used]);
	num_intersections_used += num_discs;
	ray.intersect_with_triangles(scene.triangles, num_triangles, &intersection_buffer[num_intersections_used], flip_triangle_dir);
	num_intersections_used += num_triangles;
	assert(num_intersections_used < buffer_size);
    return closest_intersection(intersection_buffer, num_intersections_used);
}

const int MAX_DEPTH = 10;
vec3 RayTracer::trace_ray(const Scene& scene, Rand& rand,
                            Intersection* i_buffer, int ibuffer_size,
						  DebugDraw& dd, const Ray& ray, int depth, bool debug, bool use_imp) const
{
	if(depth > MAX_DEPTH) return vec3(0);
	
	int closest_i = closest_intersect_ray_scene(scene, ray, i_buffer, ibuffer_size, false);
	if(closest_i == -1)
	{
		return background;
	}
	Intersection closest = i_buffer[closest_i];
	
	if(debug) dd.add_ray(ray, closest.t );
	vec3 radiance(0);
	{
		//direct lighting
		vec3 light_dir = normalize(scene.area_lights[0].sample_pt(rand) - closest.position);
		Ray shadow_ray(closest.position +  light_dir * 0.0001f, light_dir);
		int shadow_hit_i = closest_intersect_ray_scene(scene, shadow_ray, i_buffer, ibuffer_size, true);
		if(!closest.material->refraction.enabled)
		{
			bool visibility = (shadow_hit_i == -1);
			vec3 direct = (closest.material->lambert.eval() + closest.material->phong.eval(-light_dir, -ray.dir)) * 
				saturate(dot(closest.normal, light_dir)) * vec3(visibility) * scene.area_lights[0].color;
			direct /= scene.area_lights[0].pdf();
			radiance += direct;
		}

	}
	{		
		//indirect lighting
		vec3 rotaxis = cross(vec3(0, 0, 1), closest.normal);
		float rotangle = acos(dot(closest.normal, vec3(0, 0, 1)));
		mat4 rot = rotate(degrees(rotangle), rotaxis);

		vec3 wi_world;
		vec3 weight;
		if(closest.material->refraction.enabled)
		{
			vec3 r_world = reflect(ray.dir, closest.normal);
			vec3 h_world = normalize(r_world + -ray.dir);
			float half_odoth = glm::abs(dot(r_world, h_world));
			float f = closest.material->refraction.fresnel.eval(half_odoth);
			bool sample_reflection = rand.next01() < f;
			
			//if(depth == 0 && debug) cout<<"f = " << f << endl;
			if(sample_reflection)
			{
				wi_world = r_world;
				weight = vec3(dot(r_world, closest.normal));
			}
			else
			{				
				vec3 wo_local = vec3(transpose(rot) * vec4(-ray.dir, 0));
				vec3 wi_local;
				closest.material->refraction.sample(wo_local, vec2(rand.next01(), rand.next01()), &wi_local, &weight);
				wi_world = vec3(rot * vec4(wi_local, 0));
			}
		}
		else
		{
			bool sample_spec = rand.next01() > 1;
			if(sample_spec)
			{
				vec3 wo_local = vec3(transpose(rot) * vec4(-ray.dir, 0));
				vec3 wi_local;
				closest.material->phong.sample(wo_local, vec2(rand.next01(), rand.next01()), &wi_local, &weight);		
				wi_world = vec3(rot * vec4(wi_local, 0));
			}
			else
			{
				vec3 wi_local;
				closest.material->lambert.sample(vec2(rand.next01(), rand.next01()), &wi_local, &weight);			
				wi_world = vec3(rot * vec4(wi_local, 0));
			
			}
		}
		
		Ray wi_ray = Ray(closest.position + 0.0001f * wi_world, wi_world);   
		
		vec3 wi_l = trace_ray(scene, rand, i_buffer, ibuffer_size, dd, wi_ray, depth+1, debug, use_imp);
		radiance += wi_l * weight;
	}
    
    return radiance;
}
int RayTracer::raytrace(DebugDraw& dd, int total_groups, int my_group, int groups_n, bool clear_fb)
{
	float zn = 1; float zf = 50;
	int rays_pp = 1;
	mat4 inv_view = inverse(camera.view());
	mat4 inv_proj = inverse(camera.projection());
	//Intersection intersections[2];
	vec3 o = camera.eye;
	const int ibuffer_size = 128;
	Intersection i_buffer[ibuffer_size];
	//for(int i = 0; i < num_spheres; i++) dd.add_sphere(scene.spheres[i], vec3(.1, 0, 0));
	dd.add_disc(scene.discs[0], vec3(0, 1, 0));
	
    int i_start = (int)floor((float)my_group/total_groups * h);
    int i_end = (int)floor((float)(my_group+1)/total_groups * h);
    
    //printf("thread %d processing from %d to %d\n", my_group, i_start, i_end);
    int rays_traced = 0;

	for(int i = 0; i < h; i++)
	{
		if((i % groups_n) != my_group) continue;
        rays_traced += w * rays_pp * (MAX_DEPTH + 1);
		for(int j = 0; j < w; j++)
		{
            bool debug = false;
            int fb_i = i * w + j;
            if(j == debug_pixel.x && i == debug_pixel.y)
			{
				debug = true;
			}
			
			vec2 pixel_pos(j, i);
			
			vec2 sample_pos = pixel_pos + vec2(rand[my_group].next01(), rand[my_group].next01());
					
			vec4 ndc((sample_pos.x/w-0.5)*2, (sample_pos.y/h-0.5)*2, 1, 1);
			vec4 d_comp = inv_proj * ndc;
			d_comp = d_comp/d_comp.w;
			d_comp = inv_view * d_comp;
			vec3 d = normalize(vec3(d_comp));

			Ray ray(o, d);
			vec3 color = trace_ray(scene, rand[my_group], i_buffer, ibuffer_size, dd, ray, 0, debug, true);//j > w/2);
			

			
            vec3 existing_color = vec3(linear_fb[fb_i]);
            float samples_count = linear_fb[fb_i].w + 1;
            if(clear_fb) samples_count = 1;
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
	}
	dd.flip();
    return rays_traced;
}

void RayTracer::resize(int w, int h)
{
    this->w = w; this->h = h;
}