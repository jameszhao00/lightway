#pragma once

#include <glm/glm.hpp>
#include <AntTweakBar.h>
#include "rendering.h"
#include "shapes.h"
#include "lwmath.h"
#include "uniformgrid.h"
#include "SampleHistoryRecorder.h"

const int num_spheres = 2;
const int num_discs = 1;
const int num_lights = 1;
const int num_area_lights = 1;
struct Light
{
	Light () : color(1) { }
	float3 position;
	float3 color;
};
struct RectangularAreaLight
{
    RectangularAreaLight() : normal(0), material(nullptr) { }
    float3 corners[4];
    float3 normal;
    //float3 color;
	const Material *material;
    float3 sample_pt(Rand& rand) const
    {
        float r0 = rand.norm_unif_rand(rand.gen);
        float r1 = rand.norm_unif_rand(rand.gen);
        
        float3 pt0 = (corners[0] - corners[3]) * r0 + corners[3];
        float3 pt1 = (corners[1] - corners[2]) * r0 + corners[2];
        float3 pt = r1 * (pt1 - pt0) + pt0;
        return pt;    
    }
    float pdf() const
    {
        return 1.0f / area();
    }
    float area() const
    {
        return length(corners[0] - corners[3]) * length(corners[1] - corners[2]);
    }
	bool intersect(const Ray& ray, Intersection* intersection, bool flip_ray) const
	{
		//normally, ray and normal's directions have to be opposite
		if(dot(float3(flip_ray ? -1 : 1) * ray.dir, normal) > 0) return false;
		float rdotn = dot(ray.dir, normal);
		const float epsilon = 0.0001f;
		if(fabs(rdotn) < epsilon) return false;

		float d = dot(corners[0] - ray.origin, normal) / rdotn;

		if(d < 0) return false;

		float3 pt = ray.at(d);
		float dp = dot(normalize(pt - corners[3]), normalize(corners[0] - corners[3]));
		if((dot(normalize(pt - corners[0]), normalize(corners[1] - corners[0]))) < -epsilon) return false;
		if((dot(normalize(pt - corners[1]), normalize(corners[2] - corners[1]))) < -epsilon) return false;
		if((dot(normalize(pt - corners[2]), normalize(corners[3] - corners[2]))) < -epsilon) return false;
		if((dot(normalize(pt - corners[3]), normalize(corners[0] - corners[3]))) < -epsilon) return false;

		intersection->hit = true;
		intersection->material = material;
		intersection->normal = normal;
		intersection->position = pt;
		intersection->t = d;
		return true;
	} 
	//not used
	bool is_valid()
	{
		return material != nullptr && normal != float3(0);
	}
};
struct RTScene
{
	RTScene() : scene(nullptr)
	{
		materials[0].lambert.albedo = float3(.8, .2, .2);	
		materials[0].phong.spec_power = 10;
		materials[0].phong.f0 = float3(.08);
		materials[1].lambert.albedo = float3(1);	
		materials[1].phong.spec_power = 100;
    		materials[2].lambert.albedo = float3(.2, .2, .8);	
    		materials[2].phong.spec_power = 10;
    		materials[2].phong.f0 = float3(.08);

			auto fresnel = &materials[0].refraction.fresnel;
			fresnel->eta_inside = 1.3333f;
			fresnel->eta_outside = 1;
			fresnel->cache_f0();

			materials[0].refraction.enabled = false;

		spheres[0] = Sphere(float3(0, 1, -5), 1, &materials[0]);
		spheres[1] = Sphere(float3(2.2, 1, -5), 1, &materials[2]);
		discs[0] = Disc(float3(0, 0, 0), normalize(float3(.01, 1, 0)), 30, &materials[1]);

		lights[0].position = float3(-4, 10, 0);
		lights[0].color = float3(1);
		float offsetX = -24;
		float offsetZ = -20;
		/*
		float3 light_verts[] = {
			float3(343.f/555*48+offsetX, 39.5, 227.f/555*48+offsetZ),		
			float3(343.f/555*48+offsetX, 39.5, 332.f/555*48+offsetZ),
			float3(213.f/555*48+offsetX, 39.5, 332.f/555*48+offsetZ),	
			float3(213.f/555*48+offsetX, 39.5, 227.f/555*48+offsetZ)
		};*/
		
		float3 light_verts[] = {
			float3(-.25, .35, -.25),
			float3(-.25, .35, .25),
			float3(.25, .35, .25),
			float3(.25, .35, -.25)
		};
        area_lights[0].corners[0] = light_verts[0];//float3(-1, 39.5, -1);
        area_lights[0].corners[1] = light_verts[1];//float3(1, 39.5, -1);
        area_lights[0].corners[2] = light_verts[2];//float3(1, 39.5, 1);
        area_lights[0].corners[3] = light_verts[3];//float3(-1, 39.5, 1);
        area_lights[0].normal = float3(0, -1, 0);
		light_material.emission = float3(5);
		area_lights[0].material = &light_material;

		triangles.push_back(Triangle(
			float3(-1, .2, -1),
            float3(1, .2, -1), 
            float3(0, .2, 1), 
            normalize(float3(0.01, 1, 0)),
            &materials[0]));
           

	}
	void make_accl()
	{		
		accl = make_uniform_grid(*(this->scene), int3(20));
	}
	Material light_material;
    RectangularAreaLight area_lights[num_area_lights];
	Sphere spheres[num_spheres];
	Disc discs[num_discs];
	Material materials[4];
	Light lights[num_lights];
	vector<Triangle> triangles;
	unique_ptr<UniformGrid> accl;
	const StaticScene* scene;
};
class DebugDraw;
const int FB_CAPACITY = 2048;

struct Sample
{
	Sample() { reset(); }
	Ray ray;
	float3 throughput;
	float3 radiance;
	int2 xy;
	bool finished;
	bool specular_using_brdf;
	int depth;
	void reset()
	{
		depth = 0;
		radiance = float3(0); 
		throughput = float3(1);
		finished = false;
		specular_using_brdf = false;
		xy = int2(0);
	}
};
class RayTracer
{    
public:    
	void init()
	{
		use_fresnel = true;
	}
	RayTracer() : background(0)//135.0f/255, 180.0f/255, 250.0f/255) 
	{ 
        linear_fb = new float4[FB_CAPACITY * FB_CAPACITY];
        sample_fb = new Sample[FB_CAPACITY * FB_CAPACITY];
	}
    ~RayTracer() 
	{ 
		delete [] linear_fb;
		delete [] sample_fb;
	}
	//returns rays count
	void RayTracer::process_samples(const RTScene& scene, Rand& rand, Sample* samples_array, int sample_n, SampleDebugger& sd);
    int raytrace(int total_groups, int my_group, int group_n, bool clear_fb, SampleDebugger& sd);
    void resize(int w, int h);

	Camera camera;
    float4* linear_fb;
	Sample* sample_fb;
    //Color* fb;
    int w; int h;
    int2 debug_pixel;
	float3 background;
	RTScene* scene;
    Rand rand[32];
	bool use_fresnel;
	int spp;
private:
    RayTracer(const RayTracer& other) { assert(false); }
    RayTracer& operator=(const RayTracer& other) { assert(false); }
};