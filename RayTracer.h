#pragma once

#include <glm/glm.hpp>
#include <AntTweakBar.h>
#include "rendering.h"
#include "shapes.h"
#include "math.h"
#include "shading/material.h"
#include "uniformgrid.h"
using namespace glm;
struct Color
{
	unsigned char r, g, b, a;
};

const int num_spheres = 2;
const int num_discs = 1;
const int num_lights = 1;
const int num_area_lights = 1;
struct Light
{
	Light () : color(1) { }
	vec3 position;
	vec3 color;
};
struct RectangularAreaLight
{
    RectangularAreaLight() : normal(0), material(nullptr) { }
    vec3 corners[4];
    vec3 normal;
    //vec3 color;
	const Material *material;
    vec3 sample_pt(Rand& rand) const
    {
        float r0 = rand.norm_unif_rand(rand.gen);
        float r1 = rand.norm_unif_rand(rand.gen);
        
        vec3 pt0 = (corners[0] - corners[3]) * r0 + corners[3];
        vec3 pt1 = (corners[1] - corners[2]) * r0 + corners[2];
        vec3 pt = r1 * (pt1 - pt0) + pt0;
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
		if(dot(vec3(flip_ray ? -1 : 1) * ray.dir, normal) > 0) return false;
		float rdotn = dot(ray.dir, normal);
		if(rdotn == 0) return false;

		float d = dot(corners[0] - ray.origin, normal) / rdotn;
		vec3 pt = ray.at(d);
		if(dot(normalize(pt - corners[0]), normalize(corners[1] - corners[0])) < 0) return false;
		if(dot(normalize(pt - corners[1]), normalize(corners[2] - corners[1])) < 0) return false;
		if(dot(normalize(pt - corners[2]), normalize(corners[3] - corners[2])) < 0) return false;
		if(dot(normalize(pt - corners[3]), normalize(corners[0] - corners[3])) < 0) return false;

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
		return material != nullptr && normal != vec3(0);
	}
};
struct RTScene
{
	RTScene() : scene(nullptr)
	{
		materials[0].lambert.albedo = vec3(.8, .2, .2);	
		materials[0].phong.spec_power = 10;
		materials[0].phong.f0 = vec3(.08);
		materials[1].lambert.albedo = vec3(1);	
		materials[1].phong.spec_power = 100;
    		materials[2].lambert.albedo = vec3(.2, .2, .8);	
    		materials[2].phong.spec_power = 10;
    		materials[2].phong.f0 = vec3(.08);

			auto fresnel = &materials[0].refraction.fresnel;
			fresnel->eta_inside = 1.3333f;
			fresnel->eta_outside = 1;
			fresnel->cache_f0();

			materials[0].refraction.enabled = false;

		spheres[0] = Sphere(vec3(0, 1, -5), 1, &materials[0]);
		spheres[1] = Sphere(vec3(2.2, 1, -5), 1, &materials[2]);
		discs[0] = Disc(vec3(0, 0, 0), normalize(vec3(.01, 1, 0)), 30, &materials[1]);

		lights[0].position = vec3(-4, 10, 0);
		lights[0].color = vec3(1);
		float offsetX = -24;
		float offsetZ = -20;
		/*
		vec3 light_verts[] = {
			vec3(343.f/555*48+offsetX, 39.5, 227.f/555*48+offsetZ),		
			vec3(343.f/555*48+offsetX, 39.5, 332.f/555*48+offsetZ),
			vec3(213.f/555*48+offsetX, 39.5, 332.f/555*48+offsetZ),	
			vec3(213.f/555*48+offsetX, 39.5, 227.f/555*48+offsetZ)
		};*/
		
		vec3 light_verts[] = {
			vec3(-1, 4, -1),
			vec3(-1, 4, 1),
			vec3(1, 4, 1),
			vec3(1, 4, -1)
		};
        area_lights[0].corners[0] = light_verts[0];//vec3(-1, 39.5, -1);
        area_lights[0].corners[1] = light_verts[1];//vec3(1, 39.5, -1);
        area_lights[0].corners[2] = light_verts[2];//vec3(1, 39.5, 1);
        area_lights[0].corners[3] = light_verts[3];//vec3(-1, 39.5, 1);
        area_lights[0].normal = vec3(0, -1, 0);
		light_material.emission = vec3(5);
		area_lights[0].material = &light_material;

		triangles.push_back(Triangle(
			vec3(-1, .2, -1),
            vec3(1, .2, -1), 
            vec3(0, .2, 1), 
            normalize(vec3(0.01, 1, 0)),
            &materials[0]));
           

	}
	void make_accl()
	{		
		accl = make_uniform_grid(*(this->scene), ivec3(20));
	}
	Material light_material;
	TwBar *bar;
	void init_tweaks();
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
	vec3 throughput;
	vec3 radiance;
	bool finished;
	int depth;
	void reset()
	{
		depth = 0;
		radiance = vec3(0); 
		throughput = vec3(1);
		finished = false;
	}
};
class RayTracer
{    
public:    
	void init()
	{
		scene.init_tweaks();
		use_fresnel = true;
		TwAddVarRW(scene.bar, "Fresnel", TW_TYPE_BOOLCPP, &use_fresnel, "");
	}
	RayTracer() : background(135.0f/255, 180.0f/255, 250.0f/255) 
	{ 
	    fb = new Color[FB_CAPACITY * FB_CAPACITY]; 
        linear_fb = new vec4[FB_CAPACITY * FB_CAPACITY];
        sample_fb = new Sample[FB_CAPACITY * FB_CAPACITY];
	}
    ~RayTracer() { delete [] fb; }
	//returns rays count
	void RayTracer::process_samples(const RTScene& scene, Rand& rand, Sample* samples_array, int sample_n, bool debug);
    int raytrace(DebugDraw& dd, int total_groups, int my_group, int group_n, bool clear_fb);
    void resize(int w, int h);
	/*
	vec3 trace_ray(const RTScene& scene, 
        Rand& rand,
	    Intersection* i_buffer, 
	    int ibuffer_size, DebugDraw& dd, const Ray& ray, int depth, bool debug,
	    bool use_imp) const;
		*/
	Camera camera;
    vec4* linear_fb;
	Sample* sample_fb;
    Color* fb;
    int w; int h;
    ivec2 debug_pixel;
	vec3 background;
	RTScene scene;
    Rand rand[32];
	bool use_fresnel;
	int spp;
private:
    RayTracer(const RayTracer& other) { assert(false); }
    RayTracer& operator=(const RayTracer& other) { assert(false); }
};