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
	RTScene() : scene(nullptr) { }
	void make_accl()
	{		
		accl = move(makeAccelScene(scene.get()));// make_uniform_grid(*(this->scene), int3(30));
	}
	const RectangularAreaLight* light(int lightIdx) const
	{
		return &accl->lights[lightIdx];
	}
	unique_ptr<AccelScene> accl;
	unique_ptr<StaticScene> scene;
};
const int FB_CAPACITY = 1024;

struct Sample
{
	Sample() { reset(); }
	Ray ray;
	float3 radiance;
	int2 xy;
	void reset()
	{
		radiance = float3(0); 
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
	void processSampleToCompletionMis(Rand& rand, Sample* sample);
	void bidirectionallyProcessSampleToCompletion(Rand& rand, Sample* sample);
    int step(Rand& rand, int groupIdx);
	void processSampleToCompletion(Rand& rand, Sample* sample);
private:
    int2 size_;
	bool clearSignal_[MAX_RENDER_THREADS];
	int cachedWorkThreadN_;
	bool stopSignal_;
	vector<boost::thread> workThreads_;
	SampleDebugger sampleDebugger_;
	int maxDepth_;
	bool includeDirect_;
	int debugExclusiveBounce_;
};