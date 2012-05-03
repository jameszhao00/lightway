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
const float MIN_RAY_NORMAL_ANGLE = 0.0001;
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
	void sample(Rand& rand, const float3& pos, float3* lightPos, float3* wiWorld, float* pdf, float* t) const
	{
		*lightPos = sample_pt(rand);
		*wiWorld = normalize(*lightPos - pos);
		*t = glm::length(*lightPos - pos);
		*pdf = (*t * *t) / (dot(normal, -*wiWorld) * area());
	}
	float pdf(const float3& wi, float d)
	{
		return (d * d) / (zup::cos_theta(wi) * area());
	}
    float area() const
    {
        return length(corners[0] - corners[3]) * length(corners[1] - corners[2]);
    }
	bool intersect(const Ray& ray, Intersection* intersection, bool flip_ray) const
	{
		//normally, ray and normal's directions have to be opposite
		if(dot(float3(flip_ray ? -1 : 1) * ray.dir, normal) > MIN_RAY_NORMAL_ANGLE) return false;
		float rdotn = dot(ray.dir, normal);
		const float epsilon = 0.0001f;
		if(fabs(rdotn) < MIN_RAY_NORMAL_ANGLE) return false;

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
		intersection->normal = (normal);
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
		float3 base(.5, .5, 0);/*
		float3 light_verts[] = {
			float3(-.125, .3, -.125),
			float3(-.125,.3, .125),
			float3(.125, .3, .125),
			float3(.125, .3, -.125)
		};*/
		
		float3 light_verts[] = {
			base + float3(0, .025, .025),
			base + float3(0, .025, -.025),
			base + float3(0, -.025, -.025),
			base + float3(0, -.025, .025)
		};
		/*
		float3 light_verts[] = {
			base + float3(-1, 3, 1),
			base + float3(-1, 3, -1),
			base + float3(1, 3, -1),
			base + float3(1, 3, 1)
		};*/
        area_lights[0].corners[0] = light_verts[0];//float3(-1, 39.5, -1);
        area_lights[0].corners[1] = light_verts[1];//float3(1, 39.5, -1);
        area_lights[0].corners[2] = light_verts[2];//float3(1, 39.5, 1);
        area_lights[0].corners[3] = light_verts[3];//float3(-1, 39.5, 1);
        area_lights[0].normal = float3(-1, 0, 0);
		light_material.emission = float3(350);
		area_lights[0].material = &light_material;
	}
	void make_accl()
	{		
		accl = make_uniform_grid(*(this->scene), int3(30));
	}
	Material light_material;
    RectangularAreaLight area_lights[num_area_lights];
	Sphere spheres[num_spheres];
	Disc discs[num_discs];
	Material materials[4];
	Light lights[num_lights];
	vector<Triangle> triangles;
	unique_ptr<UniformGrid> accl;
	unique_ptr<StaticScene> scene;
};
const int FB_CAPACITY = 1024;

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
const int MAX_RENDER_THREADS = 8;
#include <boost/thread.hpp>
class RenderCore
{    
public:    
	RenderCore();
    ~RenderCore();
public:
    void resize(const int2& size);
	void clear();
	void startWorkThreads();
	void stopWorkThreads();
	SampleDebugger& sampleDebugger() { return sampleDebugger_; }
	
	Camera camera;
    float4* linear_fb;
	float3 background;
	RTScene* scene;
private:
    RenderCore(const RenderCore& other);
    RenderCore& operator=(const RenderCore& other);
	void workThread(int groupIdx);
	
	void processSample(Rand& rand, Sample* sample);
    int step(Rand& rand, int groupIdx);
private:
    int2 size_;
	bool clearSignal_[MAX_RENDER_THREADS];
	int cachedWorkThreadN_;
	bool stopSignal_;
	vector<boost::thread> workThreads_;
	SampleDebugger sampleDebugger_;
};