#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <AntTweakBar.h>
#include "rendering.h"
#include "shapes.h"
#include "math.h"
#include "shading/material.h"
using namespace glm;
struct Color
{
	unsigned char r, g, b, a;
};

const int num_spheres = 2;
const int num_discs = 1;
const int num_lights = 1;
const int num_area_lights = 1;
const int num_triangles = 1;
struct Light
{
	Light () : color(1) { }
	vec3 position;
	vec3 color;
};
struct RectangularAreaLight
{
    RectangularAreaLight() : color(1) { }
    vec3 corners[4];
    vec3 normal;
    vec3 color;
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
};
struct Scene
{
	Scene()
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

			materials[0].refraction.enabled = true;

		spheres[0] = Sphere(vec3(0, 1, -5), 1, &materials[0]);
		spheres[1] = Sphere(vec3(2.2, 1, -5), 1, &materials[2]);
		discs[0] = Disc(vec3(0, 0, 0), normalize(vec3(.01, 1, 0)), 30, &materials[1]);

		lights[0].position = vec3(-4, 10, 0);
		lights[0].color = vec3(1);
		
        area_lights[0].corners[0] = vec3(-1, 2, -1);
        area_lights[0].corners[1] = vec3(1, 2, -1);
        area_lights[0].corners[2] = vec3(1, 2, 1);
        area_lights[0].corners[3] = vec3(-1, 2, 1);
        area_lights[0].normal = vec3(0, -1, 0);
        area_lights[0].color = vec3(25);

        triangles[0] = Triangle(vec3(-1, .2, -1),
            vec3(1, .2, -1), 
            vec3(0, .2, 1), 
            normalize(vec3(0.01, 1, 0)),
            &materials[0]);
            

	}
	TwBar *bar;
	void init_tweaks()
	{		 
		bar = TwNewBar("Material");
		TwAddVarRW(bar, "Ball Albedo", TW_TYPE_COLOR3F, value_ptr(materials[0].lambert.albedo), "");
		TwAddVarRW(bar, "Ball Roughness", TW_TYPE_FLOAT, &materials[0].phong.spec_power, "");
		TwAddVarRW(bar, "Ball Specular", TW_TYPE_COLOR3F, value_ptr(materials[0].phong.f0), "");
		
		TwAddVarRW(bar, "Ball2 Albedo", TW_TYPE_COLOR3F, value_ptr(materials[2].lambert.albedo), "");
		TwAddVarRW(bar, "Ball2 Roughness", TW_TYPE_FLOAT, &materials[2].phong.spec_power, "");
		TwAddVarRW(bar, "Ball2 Specular", TW_TYPE_COLOR3F, value_ptr(materials[2].phong.f0), "");
	}
    RectangularAreaLight area_lights[num_area_lights];
	Sphere spheres[num_spheres];
	Disc discs[num_discs];
	Material materials[4];
	Light lights[num_lights];
	Triangle triangles[num_triangles];
};
class DebugDraw;
const int FB_CAPACITY = 2048;
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
	}
    ~RayTracer() { delete [] fb; }
	//returns rays count
    int raytrace(DebugDraw& dd, int total_groups, int my_group, int group_n, bool clear_fb);
    void resize(int w, int h);
	vec3 trace_ray(const Scene& scene, 
        Rand& rand,
	    Intersection* i_buffer, 
	    int ibuffer_size, DebugDraw& dd, const Ray& ray, int depth, bool debug,
	    bool use_imp) const;
	Camera camera;
    vec4* linear_fb;
    Color* fb;
    int w; int h;
    ivec2 debug_pixel;
	vec3 background;
	Scene scene;
    Rand rand[32];
	bool use_fresnel;
	int spp;
private:
    RayTracer(const RayTracer& other) { assert(false); }
    RayTracer& operator=(const RayTracer& other) { assert(false); }
};