#include "stdafx.h"
#include "RenderCore.h"
#include "bxdf.h"
#include "shapes.h"
#include <glm/ext.hpp>
#include <iostream>

//e.g. if MAX_LIGHT_PATH_DEPTH is 2
//eye isect[0]/throughput[0] ----throughput[1]---- isect[1] ----throughput[2]---- isect[2]
const int MAX_LIGHT_PATH_DEPTH = 6;

//returns # of isects
//PRE CONDITION
//capacity of lightPathIsect/lightPathThroughputs = MAX_LIGHT_PATH_DEPTH + 1
//POST CONDITION
//lightPathIsect filled with MAX_LIGHT_PATH_DEPTH + 1 intersections
//lightPathThroughputs filled with MAX_LIGHT_PATH_DEPTH + 1 throughputs... each describing transfer from 0->i
//lightPathIsect[0] = starting point at light
//lightPathThroughputs[0] = light emission * light area
int createLightPath2(const RTScene* scene, Rand& rand, Intersection isects[], float3 T[])
{
	const auto & light = *scene->light(0);
	float3 lightPos = light.samplePoint(rand);
	ShadingCS lightPosShadingCS(light.normal);
	float3 woWorldLight = lightPosShadingCS.world(sampleHemisphere(rand));

	T[0] = light.material.emission * light.area();
	isects[0].position = lightPos; 
	isects[0].normal = light.normal;

	float3 lightPdf = float3(1.f / (2 * PI * light.area()));
	T[1] = light.material.emission * dot(woWorldLight, light.normal) / lightPdf;
	int numIsects = 1;
	Ray ray(lightPos, woWorldLight);
	for(int depth = 1; depth < (MAX_LIGHT_PATH_DEPTH + 1); depth++)
	{		
		IntersectionQuery query(ray);
		Intersection isect;
		intersectScene(*scene, query, &isect, nullptr);
		if(!isect.hit) break;
		else numIsects++;
		isects[depth] = isect;
		if(depth > 1)		
		{
			float pdf = 1 / (2*PI*dot(isects[depth-1].normal, ray.dir));
			ShadingCS prevIsectShadingCS(isects[depth-1].normal);
			float3 wi = prevIsectShadingCS.local(ray.dir);
			float3 wo = prevIsectShadingCS.local(normalize(isects[depth-2].position - isects[depth-1].position));
			float3 brdfEval = isects[depth-1].material->eval(wi, wo);
			T[depth] = T[depth - 1] * brdfEval / pdf;
		}

		if(depth == MAX_LIGHT_PATH_DEPTH) break;

		ray.origin = isect.position;		
		ShadingCS isectShadingCS(isect.normal);
		ray.dir = isectShadingCS.world(sampleHemisphere(rand));
		
	}
	return numIsects;
}


void RenderCore::bidirectionallyProcessSampleToCompletion(Rand& rand, Sample* sample)
{
	//const float rrStartDepth = glm::min(glm::max(maxDepth_ / 2, 3), 5);
	Intersection lightPathIsect[MAX_LIGHT_PATH_DEPTH + 1];
	//for throughput[i], we don't include the weight at i
	float3 lightPathThroughputs[MAX_LIGHT_PATH_DEPTH + 1]; //the throughput from vert i -> 0
	int lightPathIsectsN = createLightPath2(scene, rand, lightPathIsect, lightPathThroughputs);
	float3 throughput(1);
	//trace the eye ray 
	Ray sceneRay = sample->ray;
	for(int depth = 0; depth < (bounces_ + 1); depth++)
	{
		Intersection sceneIsect;
		Intersection lightisect;
		IntersectionQuery sceneIsectQuery(sceneRay);
		intersectScene(*scene, sceneIsectQuery, &sceneIsect, &lightisect);
		//we hit a light
		if(hitLightFirst(sceneIsect, lightisect))
		{
			if((includeDirect_ || depth != 1))
				//&& debugExclusiveBounce_ == (depth))
			{
				//there's NO shadow ray here! (so not depth + 1 + 1)
				int pathLength = depth + 1;
				
				float bdptWeight = (1.f / (pathLength));
				sample->radiance += bdptWeight * throughput * lightisect.material->emission;
			}
		}
		if(!sceneIsect.hit) return; //we didn't hit scene
		if(depth == bounces_) 
		{
			//we only came here for the direct connection...
			break;
		}
		
		ShadingCS evShadingCS(sceneIsect.normal);
		float3 woEV = evShadingCS.local(-sceneRay.dir);
		for(int lightPathIsectIdx = 0; lightPathIsectIdx < lightPathIsectsN; lightPathIsectIdx++)			
		{
			//there's a shadow ray here! (so +1)
			int pathLength = (lightPathIsectIdx) + (depth + 1) + 1;
			//todo: make the following depend on Russian Roulette data
			//we lose a variability b/c our first eye path is not removable

			float bidirectionalWeight = 1.f / (pathLength);
			
			
			if(pathLength > (bounces_ + 1)) continue;

			if(visibleAndFacing(lightPathIsect[lightPathIsectIdx], sceneIsect, *scene)
				&& (includeDirect_ || (depth != 0 || lightPathIsectIdx != 0)))
			{
				const auto & lv = lightPathIsect[lightPathIsectIdx];
				const auto & ev = sceneIsect;
				//light vert to eye vert dir
				float3 ev2lv = normalize(lv.position - ev.position);
				//eye vert brdf
				float3 wiEV = evShadingCS.local(ev2lv);
				float3 evBrdfEval = ev.material->eval(wiEV, woEV);
				//light vert brdf
				float3 lvBrdfEval;
				if(lightPathIsectIdx == 0)
				{
					lvBrdfEval = float3(1);
				}
				else
				{				
					ShadingCS lvShadingCS(lv.normal);
					float3 wiLV = lvShadingCS.local(-ev2lv);
					float3 woLV = lvShadingCS.local(normalize(lightPathIsect[lightPathIsectIdx - 1].position 
						- lightPathIsect[lightPathIsectIdx].position));
					lvBrdfEval = lv.material->eval(wiLV, woLV);
				}
						
				//geometry term
				float g = dot(ev2lv, ev.normal) * dot(-ev2lv, lv.normal) / glm::distance2(lv.position, ev.position);
				float3 lpT = lvBrdfEval * lightPathThroughputs[lightPathIsectIdx];
				float3 epT = evBrdfEval * throughput;
				float3 T = lpT * epT * g;
				
				sample->radiance += bidirectionalWeight * T;
			}
			
		}
		//use brdf and sample a new direction direction 
		float3 wiIndirect;
		float3 brdfWeight;
		sceneIsect.material->sample(woEV, float2(rand.next01(), rand.next01()), &wiIndirect, &brdfWeight);
		float3 wiIndirectWorld = evShadingCS.world(wiIndirect);
		sceneRay = Ray(sceneIsect.position, wiIndirectWorld);
		//update throughput	
		throughput *= brdfWeight;
		//russian roulette
		/*
		if(depth > rrStartDepth)
		{
			float survivalProb = glm::min(luminance(throughput), 0.5f);
			if(rand.next01() > survivalProb) break;
			else throughput /= survivalProb;//glm::min(survivalProb * 2.f, 1.f);
		}
		*/
	}
}
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
			
			int desiredBounces =8;
			includeDirect_ = true;
			debugExclusiveBounce_ = 0;
			if(1)
			{
				bounces_ = desiredBounces;
				ptMISRun(*scene, bounces_, rand, &sample);
			}
			else if(0)
			{
				bounces_ = desiredBounces;
				ptRun(*scene, bounces_, rand, &sample);
			}
			else 
			{
				bounces_ = desiredBounces + 1;
				bidirectionallyProcessSampleToCompletion(rand, &sample);
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
	while(!stopSignal_)
		//&& boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::system_clock::now() - start).
		//count() < 15000)
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

bool visibleAndFacing( const Intersection& isectA, const Intersection& isectB, RTScene& scene )
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
