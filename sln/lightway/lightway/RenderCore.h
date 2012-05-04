#pragma once

#include <glm/glm.hpp>
#include <AntTweakBar.h>
#include "rendering.h"
#include "shapes.h"
#include "lwmath.h"
#include "uniformgrid.h"
#include "SampleHistoryRecorder.h"
#include "AreaLight.h"
#include "scene.h"
const int num_spheres = 2;
const int num_discs = 1;
const int num_lights = 1;
const int num_area_lights = 1;
struct RTScene
{
	RTScene() : scene(nullptr)
	{	
		float3 base(.5, .5, 0);
		
		float3 light_verts[] = {
			float3(-.125, .3, -.125),
			float3(-.125,.3, .125),
			float3(.125, .3, .125),
			float3(.125, .3, -.125)
		};
		
		/*
		float3 light_verts[] = {
			base + float3(0, .025, .025),
			base + float3(0, .025, -.025),
			base + float3(0, -.025, -.025),
			base + float3(0, -.025, .025)
		};
		
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
        area_lights[0].normal = float3(0, -1, 0);
		area_lights[0].material.emission = float3(1);
	}
	void make_accl()
	{		
		accl = move(makeAccelScene(scene.get()));// make_uniform_grid(*(this->scene), int3(30));
	}
	const RectangularAreaLight* light(int lightIdx)
	{
		return &area_lights[lightIdx];
	}
	Material light_material;
    RectangularAreaLight area_lights[num_area_lights];
	Sphere spheres[num_spheres];
	Disc discs[num_discs];
	Material materials[4];
	vector<Triangle> triangles;
	unique_ptr<AccelScene> accl;
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
	void processSampleToCompletion(Rand& rand, Sample* sample);
	void bidirectionallyProcessSampleToCompletion(Rand& rand, Sample* sample);
    int step(Rand& rand, int groupIdx);
private:
    int2 size_;
	bool clearSignal_[MAX_RENDER_THREADS];
	int cachedWorkThreadN_;
	bool stopSignal_;
	vector<boost::thread> workThreads_;
	SampleDebugger sampleDebugger_;
};