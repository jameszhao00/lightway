#include "stdafx.h"
#include "RenderCore.h"
#include "bxdf.h"
#include "shapes.h"
#include <glm/ext.hpp>
#include <iostream>

int RenderCore::step(Rand& rand, int groupIdx)
{
	const Camera* active_camera = &camera;

	float4x4 inv_view = glm::inverse(active_camera->view());
	float4x4 inv_proj = glm::inverse(active_camera->projection());
	float3 o = active_camera->eye;
		
    int samples_n = 0;
	bool clear = clearSignal_[groupIdx];
	for(int i = 0; i < size_.y; i++)
	{
		if((i % cachedWorkThreadN_) != groupIdx) continue;
		for(int j = 0; j < size_.x; j++)
		{
            int fb_i = i * size_.x + j;			

			Sample sample;
			sample.reset();
			
			if(clear)
			{
				linear_fb[fb_i] = float4(0);
			}
				
			float2 pixel_pos(j, i);			
			float2 sample_pos = pixel_pos + float2(rand.next01(), rand.next01());					
			float4 ndc((sample_pos.x/size_.x-0.5)*2, (sample_pos.y/size_.y-0.5)*2, 1, 1);
			float4 d_comp = inv_proj * ndc;
			d_comp = d_comp/d_comp.w;
			d_comp = inv_view * d_comp;
			float3 d = normalize(float3(d_comp));
			Ray ray(o, d);

			sample.ray = ray;
			sample.xy = int2(j, i);
			
			sampleDebugger_.shr.newSample(sample.xy);
			
			int bounces = 8 ;
			if(1)
			{
				ptMISRun(*scene, bounces, rand, &sample, false);
			}
			else if(0)
			{
				ptRun(*scene, bounces, rand, &sample, false);
			}
			else if(0)
			{
				bdptRun(*scene, bounces, rand, &sample);
			}
			else 
			{
				bdptMisRun(*scene, bounces, active_camera->forward, rand, &sample);
			}
			

			samples_n += 1;
			float3 color = sample.radiance;
			sampleDebugger_.shr.record(sample.xy, 7, "Radiance", color);
			
			float3 existing_color = float3(linear_fb[fb_i]);
			sampleDebugger_.shr.record(sample.xy, 7, "Existing Mix", existing_color);
			float samples_count = linear_fb[fb_i].w + 1;

			float3 mixed_color = float3((samples_count - 1) / samples_count) * existing_color + 
				float3(1/samples_count) * color;
			sampleDebugger_.shr.record(sample.xy, 7, "New Mix", mixed_color);
			linear_fb[fb_i] = float4(mixed_color, samples_count);
			
		}
	}
	//we don't clear unconditionally b/c clear may have been enabled since we last checked
	if(clear) clearSignal_[groupIdx] = false;
    return samples_n;
}

void RenderCore::resize(const int2& size)
{    
	size_ = size;
	camera.ar = (float)size.x / size.y;
	for(int i = 0; i < MAX_RENDER_THREADS; i++)
	{
		clearSignal_[i] = true;
	}
}
void RenderCore::clear()
{    
	for(int i = 0; i < MAX_RENDER_THREADS; i++)
	{
		clearSignal_[i] = true;
	}
}
RenderCore::RenderCore() : background(0), size_(-1, -1), stopSignal_(false), cachedWorkThreadN_(-1)
{ 
    linear_fb = new float4[FB_CAPACITY * FB_CAPACITY];
	for(int i = 0; i < MAX_RENDER_THREADS; i++) clearSignal_[i] = true;
}

RenderCore::~RenderCore() 
{ 
	delete [] linear_fb;
}
void RenderCore::startWorkThreads()
{
	//HACK:
	cachedWorkThreadN_ = boost::thread::hardware_concurrency() - 1;
	for(int i = 0; i < cachedWorkThreadN_; i++)
	{
		workThreads_.push_back(boost::thread(&RenderCore::workThread, this, i));
	}
}

void RenderCore::stopWorkThreads()
{
	stopSignal_ = true;
	for(int i = 0; i < cachedWorkThreadN_; i++)
	{
		workThreads_[i].join();
	}
	stopSignal_ = false;
	workThreads_.clear();
	cachedWorkThreadN_ = -1;
}
#include <boost/chrono.hpp>
void RenderCore::workThread(int groupIdx)
{
	Rand rand(groupIdx);
	int iteration = 0;
	auto start = boost::chrono::system_clock::now();
	//HACK:
	while(!stopSignal_)
		//&& boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::system_clock::now() - start).
		//count() < 5000)
	{
		step(rand, groupIdx);
		iteration++;
		cout << "group: " << groupIdx << " iteration:" << iteration << endl;
	}
}


bool hitLightFirst( const Intersection& sceneIsect, const Intersection& lightIsect )
{
	if(!lightIsect.hit) return false;
	if(!sceneIsect.hit) return true;
	//both hit
	if(lightIsect.t < sceneIsect.t) return true;
	return false;
}

bool hitSceneFirst( const Intersection& sceneIsect, const Intersection& lightIsect )
{
	if(!sceneIsect.hit) return false;
	if(!lightIsect.hit) return true;
	//both hit
	if(sceneIsect.t < lightIsect.t) return true;
	return false;
}

bool hitNothing( const Intersection& sceneIsect, const Intersection& lightIsect )
{
	return !sceneIsect.hit && !lightIsect.hit;
}

void intersectScene( const RTScene& scene, const IntersectionQuery& query, Intersection* sceneIsect, Intersection* lightIsect )
{
	scene.accl->intersect(query, sceneIsect, lightIsect);
}

void intersectScene( const RTScene& scene, const Ray& ray, Intersection* sceneIsect, Intersection* lightIsect )
{	
	IntersectionQuery query(ray);
	intersectScene(scene, query, sceneIsect, lightIsect);
}

bool facing( const Intersection& isectA, const Intersection& isectB )
{
	float3 dir = normalize(isectB.position - isectA.position);
	if(dot(dir, isectA.normal) < 0.00001) return false;
	if(dot(dir, isectB.normal) > -0.00001) return false;
	return true;
}

bool facing( const float3& ptA, const float3& na, const float3& ptB, const float3& nb )
{
	float3 dir = normalize(ptB - ptA);
	if(dot(dir, na) < 0.00001) return false;
	if(dot(dir, nb) > -0.00001) return false;
	return true;
}

bool visibleAndFacing( const Intersection& isectA, const Intersection& isectB, const RTScene& scene )
{
	if(!facing(isectA, isectB))
	{
		return false;
	}
	//make sure they're facing each other first 
	float3 dir = normalize(isectB.position - isectA.position);
	//now trace the ray...
	Ray ray(isectA.position, dir);
	IntersectionQuery query(ray);
	//float desiredT = glm::length(isectB.position - isectA.position);

	Intersection sceneIsect;
	intersectScene(scene, query, &sceneIsect, nullptr);
	//see if we hit isect B
	if(!sceneIsect.hit) return false;
	//if(fabs(sceneIsect.t - desiredT) > 0.0001) return false;
	if(sceneIsect.primitiveId != isectB.primitiveId) return false;
	return true;
}

bool visibleAndFacing( const float3& posA, const float3& nA, 
	const float3& posB, const float3& nB, 
	const RTScene& scene )
{

	if(!facing(posA, nA, posB, nB))
	{
		return false;
	}
	//make sure they're facing each other first 
	float3 dir = normalize(posB - posA);
	//now trace the ray...
	Ray ray(posA, dir);
	IntersectionQuery query(ray);
	float desiredT = glm::length(posB - posA);

	Intersection sceneIsect;
	intersectScene(scene, query, &sceneIsect, nullptr);
	//see if we hit isect B
	if(!sceneIsect.hit) return false;
	if(fabs(sceneIsect.t - desiredT) > 0.0001) return false;
	return true;
}
