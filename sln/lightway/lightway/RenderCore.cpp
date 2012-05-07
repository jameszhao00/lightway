#include "stdafx.h"
#include "RenderCore.h"
#include "bxdf.h"
#include "shapes.h"
#include <glm/ext.hpp>
#include <iostream>


bool intersectScene(const RTScene& scene, 
	const IntersectionQuery& query,
	Intersection* intersection)
{
	*intersection = scene.accl->intersect(query);
	return intersection->hit;
}
bool facing(const Intersection& isectA, const Intersection& isectB)
{
	float3 dir = normalize(isectB.position - isectA.position);
	if(dot(dir, isectA.normal) < 0.00001) return false;
	if(dot(dir, isectB.normal) > -0.00001) return false;
	return true;
}
//visibility = facing each other, and not occluded
bool visibleAndFacing(const Intersection& isectA, const Intersection& isectB, bool ignoreLights, RTScene& scene)
{
	if(!facing(isectA, isectB))
	{
		return false;
	}
	//make sure they're facing each other first 
	float3 dir = normalize(isectB.position - isectA.position);
	//now trace the ray...
	Ray ray(isectA.position, dir);
	IntersectionQuery query(ray, ignoreLights, false);
	Intersection actualIsect;
	float desiredT = glm::length(isectB.position - isectA.position);
	bool hit = intersectScene(scene, query, &actualIsect);
	//see if we hit isect B
	if(!hit) return false;
	if(fabs(actualIsect.t - desiredT) > 0.0001) return false;
	return true;
}

float3 directUsingLightSamplingMis(Rand& rand, const RTScene& scene, const Intersection& pt, const float3x3& worldToZUp, 
	const float3x3& zUpWoWorld, const float3& wo, int* lightIdx)
{
	auto & light = scene.accl->lights[0];
	*lightIdx = 0;
	float3 lightPos;
	float3 wiDirectWorld;
	float lightPdf;
	float lightT;
	//sample the light
	light.sample(rand, pt.position, &lightPos, &wiDirectWorld, &lightPdf, &lightT);

	//see if we're occluded
	Ray shadowRay(pt.position, wiDirectWorld);
	//we cannot ignore the light here, as we need to differentiate b/t hit empty space
	//and hit light
	IntersectionQuery shadowQuery(shadowRay, false, false);	
	Intersection occluderIntersection;
	bool hitOccluder = intersectScene(scene, shadowQuery, &occluderIntersection);
	bool hitLight = hitOccluder && occluderIntersection.lightIdx == light.idx;
	if(!(hitLight && facing(pt, occluderIntersection)))
	{
		//we missed the light, or we're in shadow, or we're not facing it
		return float3(0, 0, 0);
	}
	float3 wiDirect = worldToZUp * wiDirectWorld;
	float brdfPdf = pt.material->fresnelBlend.lambertBrdf.pdf(wiDirect);
	float3 brdfEval = pt.material->fresnelBlend.lambertBrdf.eval();//.eval(wiDirect, wo);
	float3 cosTheta = float3(dot(pt.normal, wiDirectWorld));
	float3 le = light.material.emission;
	//balanced heuristic - fPdf / (fPdf + gPdf) * fPdf simplifies to 1/(fPdf + gPdf)
	return le * brdfEval * cosTheta / (lightPdf + brdfPdf);
}
float3 directUsingBrdfSamplingMis(const RectangularAreaLight& light, 
	float distance, const float3& wiWorld, const float& cosTheta, float brdfPdf, const float3& brdfEval)
{
	//if we're being called
	//we're unoccluded

	//evaluate the light pdf
	float lightPdf = light.pdf(wiWorld, distance);
	float3 le = light.material.emission;
	//combine
	return le * brdfEval * cosTheta / (lightPdf + brdfPdf);
}

void RenderCore::processSampleToCompletion(Rand& rand, Sample* sample)
{
	
	float3 throughput(1);

	Ray ray = sample->ray;
	//path trace
	for(int depth = 0; depth < (maxDepth_ + 1); depth++)
	{		
		IntersectionQuery sceneIsectQuery(ray, false, false);
		Intersection isect;
		bool hitScene = intersectScene(*scene, sceneIsectQuery, &isect);
		//if we happen to hit a light source, add Le and quit
		if(!hitScene)
		{
			sample->radiance += throughput * background;
			return;
		}	
		if(depth == 0 && hitScene && validLightIdx(isect.lightIdx))
		{
			sample->radiance = isect.material->emission;
			return;
		}	
		float3x3 zUpToWorld;
		float3x3 worldToZUp;
		axisConversions(isect.normal, &zUpToWorld, &worldToZUp);
		float3 wo = worldToZUp * -ray.dir;
		//direct light
		if((includeDirect_ || depth != 0))
			//&& ((depth + 1) == debugExclusiveBounce_))
		{
			auto & light = scene->accl->lights[0];
			float3 lightPos;
			float lightPdf; 
			//t is already incorporated into pdf
			float lightT;
			float3 wiDirectWorld;
			//sample the light
			light.sample(rand, isect.position, &lightPos, &wiDirectWorld, &lightPdf, &lightT);

			//see if we're occluded
			Ray shadowRay(isect.position, wiDirectWorld);
			//we cannot ignore the light here, as we need to differentiate b/t hit empty space
			//and hit light
			IntersectionQuery shadowQuery(shadowRay, false, false);	
			Intersection occluderIntersection;
			bool hitOccluder = intersectScene(*scene, shadowQuery, &occluderIntersection);
			bool hitLight = hitOccluder && occluderIntersection.lightIdx == light.idx;
			if(hitLight && facing(isect, occluderIntersection))
			{
				float3 wiDirect = worldToZUp * wiDirectWorld;
				float3 brdfEval = isect.material->fresnelBlend.lambertBrdf.eval();
				float3 cosTheta = float3(dot(isect.normal, wiDirectWorld));
				float3 le = light.material.emission;

				sample->radiance += throughput * le * brdfEval * cosTheta / lightPdf;
			}
		}
		//generate new direction
		if(depth != maxDepth_) //we're at the last bounce
		{
			float3 wiIndirect; 
			float3 indirectWeight;
			isect.material->fresnelBlend.lambertBrdf.sample(float2(rand.next01(), rand.next01()), &wiIndirect, &indirectWeight);
			float3 wiWorld = zUpToWorld * wiIndirect;
			ray = Ray(isect.position, wiWorld);
			throughput *= indirectWeight;
		}
	}
}
void RenderCore::processSampleToCompletionMis(Rand& rand, Sample* sample)
{
	float3 throughput(1);

	Ray woWorldRay;
	Intersection isect;
	bool hitScene = false;
	const float rrStartDepth = glm::min(glm::max(maxDepth_ / 2, 3), 5);
	//setup
	{
		//intersect with scene
		IntersectionQuery sceneIsectQuery(sample->ray, false, false);
		hitScene = intersectScene(*scene, sceneIsectQuery, &isect);
		//if we happen to hit a light source, add Le and quit
		if(hitScene && validLightIdx(isect.lightIdx))
		{
			sample->radiance = isect.material->emission;
			return;
		}
		woWorldRay = sample->ray;
	}
	//path trace
	for(int depth = 0; depth < (maxDepth_ + 1); depth++)
	{		
		//we need isect, woWorldRay, and hitScene

		//if we didn't hit anything, add background and quit
		if(!hitScene)
		{
			sample->radiance += throughput * background;
			return;
		}		
		float3x3 zUpToWorld;
		float3x3 worldToZUp;
		axisConversions(isect.normal, &zUpToWorld, &worldToZUp);
		float3 wo = worldToZUp * -woWorldRay.dir;
		//add direct light contribution using light sampling
		int lightIdx;
		if((includeDirect_ || depth != 0))
		{
			sample->radiance += throughput *
				directUsingLightSamplingMis(rand, *scene, isect, worldToZUp, zUpToWorld, wo, &lightIdx);
		}		
		//sample the brdf
		float3 weight;
		float3 wiIndirect;
		//isect.material->fresnelBlend.sample(wo, float2(rand.next01(), rand.next01()), &wiIndirect, &weight);
		isect.material->fresnelBlend.lambertBrdf.sample(float2(rand.next01(), rand.next01()), &wiIndirect, &weight);
		float brdfPdf = isect.material->fresnelBlend.lambertBrdf.pdf(wiIndirect);//, wo);
		float3 brdfEval = isect.material->fresnelBlend.lambertBrdf.eval();//wiIndirect, wo);
		float3 wiWorldIndirect = zUpToWorld * wiIndirect;
		//don't mul. throughput by weight just yet... we need to add light
		//intersect with the scene
		Ray nextWoWorldRay(isect.position, wiWorldIndirect);
		IntersectionQuery nextSceneIsectQuery(nextWoWorldRay, false, false);
		Intersection nextIsect;
		bool nextHitScene = intersectScene(*scene, nextSceneIsectQuery, &nextIsect);
		//add direct light contribution from the BRDF sample direction (with MIS)
		if((includeDirect_ || depth != 0) && nextHitScene && validLightIdx(nextIsect.lightIdx))
		{
			//if we hit a different light, quit... 
			//should probably change this later on
			if(lightIdx != nextIsect.lightIdx) return;
			else
			{		
				//add direct light contrib.
				//TODO: maybe we should mul. throughput by weight...
				sample->radiance += throughput * 
					directUsingBrdfSamplingMis(*(scene->light(nextIsect.lightIdx)), nextIsect.t, 
					wiWorldIndirect, dot(wiWorldIndirect, isect.normal), brdfPdf, brdfEval);
				//if we did indeed hit a light, quit
				return;
			}
		}
		//mul. throughput by brdf weight
		throughput *= weight;
		if(depth > rrStartDepth)
		{
			float survivalProb = glm::min(luminance(throughput), 0.5f);
			if(rand.next01() > survivalProb) break;
			else throughput /= survivalProb;//glm::min(survivalProb * 2.f, 1.f);
		}
		hitScene = nextHitScene;
		isect = nextIsect;
		woWorldRay = nextWoWorldRay;
		//todo: implement Russian Roulette here
	}
}
//e.g. if MAX_LIGHT_PATH_DEPTH is 2
//eye isect[0]/throughput[0] ----throughput[1]---- isect[1] ----throughput[2]---- isect[2]
const int MAX_LIGHT_PATH_DEPTH = 15;

//returns # of isects
//PRE CONDITION
//capacity of lightPathIsect/lightPathThroughputs = MAX_LIGHT_PATH_DEPTH + 1
//POST CONDITION
//lightPathIsect filled with MAX_LIGHT_PATH_DEPTH + 1 intersections
//lightPathThroughputs filled with MAX_LIGHT_PATH_DEPTH + 1 throughputs... each describing transfer from 0->i
//lightPathIsect[0] = starting point at light
//lightPathThroughputs[0] = light emission * light area
int createLightPath(const RTScene* scene, Rand& rand, Intersection* isects, float3* T)
{
	//we start with light isect
	int isectsN = 1;
	const float rrStartDepth = glm::min(glm::max(MAX_LIGHT_PATH_DEPTH / 2, 3), 5);
	
	//pick point on light
	const RectangularAreaLight* light = scene->light(0);
	float3 pos = light->samplePoint(rand);
	//pick direction
	float3 lightPathWo = sampleHemisphere(rand);
	float3 woWorldPrev;
	{
		float3x3 zUpToWorld;
		float3x3 worldToZUp;
		axisConversions(light->normal, &zUpToWorld, &worldToZUp);
		woWorldPrev = zUpToWorld * lightPathWo;
	}
	//TODO: learn this thing...
	float3 lightDirPdf = float3(1/(2 * PI * light->area()));
	T[0] = light->material.emission * light->area();
	float lightCosTheta = zup::cos_theta(lightPathWo);
	isects[0].material = &light->material; 
	isects[0].normal = light->normal;
	isects[0].position = pos;
	//generate light path

	for(int depth = 1; depth < (MAX_LIGHT_PATH_DEPTH + 1); depth++)
	{			
		Ray ray(pos, woWorldPrev);
		IntersectionQuery query(ray, true, false);
		//check for intersection
		Intersection sceneIsect;
		bool hit = intersectScene(*scene, query, &sceneIsect);
		//terminate if we hit nothing...
		if(!hit) break;
		else if(validLightIdx(sceneIsect.lightIdx)) break; //also break if we hit a light
		else isectsN++; //otherwise we like this intersection + record it!	 

		if(depth == 1)
		{			
			//we cannot use T[0] as that includes area
			T[1] = light->material.emission * lightCosTheta / lightDirPdf;
		}
		if(depth > rrStartDepth)
		{
			float survivalProb = glm::min(luminance(T[depth]), 0.5f);
			if(rand.next01() > survivalProb) break;
			else T[depth] /= survivalProb;
		}
		if(depth < MAX_LIGHT_PATH_DEPTH)
		{
			float3x3 zUpToWorld;
			float3x3 worldToZUp;
			axisConversions(sceneIsect.normal, &zUpToWorld, &worldToZUp);
			float3 wi;
			float3 weight;
			sceneIsect.material->fresnelBlend.lambertBrdf.sample(float2(rand.next01(), rand.next01()), &wi, &weight);
			//safe b/c depth < max depth
			T[depth+1] = T[depth] * weight;
			//we're pretending wi is wo...
			woWorldPrev = wi * zUpToWorld;
		}

		isects[depth] = sceneIsect;
	}
	return isectsN;
}
void RenderCore::bidirectionallyProcessSampleToCompletion(Rand& rand, Sample* sample)
{
	const float rrStartDepth = glm::min(glm::max(maxDepth_ / 2, 3), 5);

	Intersection lightPathIsect[MAX_LIGHT_PATH_DEPTH + 1];
	//for throughput[i], we don't include the weight at i
	float3 lightPathThroughputs[MAX_LIGHT_PATH_DEPTH + 1]; //the throughput from vert i -> 0
	int lightPathIsectsN = createLightPath(scene, rand, lightPathIsect, lightPathThroughputs);
	float3 throughput(1);
	//trace the eye ray 
	Ray sceneRay = sample->ray;
	for(int depth = 0; depth < (maxDepth_ + 1); depth++)
	{
		Intersection sceneIsect;

		IntersectionQuery sceneIsectQuery(sceneRay, false, false);
		bool hitScene = intersectScene(*scene, sceneIsectQuery, &sceneIsect);
		//we hit nothing
		if(!hitScene)
		{
			return;
		}	
		//we hit a light
		if(validLightIdx(sceneIsect.lightIdx))
		{
			if((includeDirect_ || depth != 1))
				//&& debugExclusiveBounce_ == (depth))
			{
				//there's NO shadow ray here! (so not depth + 1 + 1)
				int pathLength = depth + 1;
				sample->radiance += (1.f / (pathLength)) * throughput * sceneIsect.material->emission;
			}
			break; //if we're at the light, we can't hit anything else...
		}
		if(depth == maxDepth_) 
		{
			//we only came here for the LDE direct connection...
			break;
		}
		float3x3 worldToZUp;
		float3x3 zUpToWorld;
		axisConversions(sceneIsect.normal, &zUpToWorld, &worldToZUp);
		float3 woEyePath = worldToZUp * -sceneRay.dir;
		//for a intersection, test visibility with every lightPathIsect
		for(int lightPathIsectIdx = 0; lightPathIsectIdx < lightPathIsectsN; lightPathIsectIdx++)
		{
			//there's a shadow ray here! (so +1)
			int pathLength = (lightPathIsectIdx) + (depth + 1) + 1;
			//todo: make the following depend on Russian Roulette data
			//we lose a variability b/c our first eye path is not removable
			float bidirectionalWeight = 1.f / (pathLength);

			if(//(debugExclusiveBounce_ == pathLength - 1) &&
				visibleAndFacing(lightPathIsect[lightPathIsectIdx], sceneIsect, true, *scene)
				&& (includeDirect_ || (depth != 0 || lightPathIsectIdx != 0)))
			{
				const auto & lv = lightPathIsect[lightPathIsectIdx];
				const auto & ev = sceneIsect;
				//eye vert brdf
				float3 evBrdfEval = ev.material->fresnelBlend.lambertBrdf.eval();
				//light vert brdf
				float3 lvBrdfEval;
				if(lightPathIsectIdx == 0)
				{
					lvBrdfEval = float3(1);
				}
				else
				{
					lvBrdfEval = lv.material->fresnelBlend.lambertBrdf.eval();
				}
				
				//light vert to eye vert dir
				float3 ev2lv = normalize(lv.position - ev.position);				
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
		//sceneIsect.material->fresnelBlend.sample(wo, float2(rand.next01(), rand.next01()), &wiIndirect, &brdfWeight);
		sceneIsect.material->fresnelBlend.lambertBrdf.sample(float2(rand.next01(), rand.next01()), &wiIndirect, &brdfWeight);
		float3 wiIndirectWorld = zUpToWorld * wiIndirect;
		sceneRay = Ray(sceneIsect.position, wiIndirectWorld);
		//update throughput	
		throughput *= brdfWeight;
		//russian roulette
		if(depth > rrStartDepth)
		{
			float survivalProb = glm::min(luminance(throughput), 0.5f);
			if(rand.next01() > survivalProb) break;
			else throughput /= survivalProb;//glm::min(survivalProb * 2.f, 1.f);
		}
	}
}
int RenderCore::step(Rand& rand, int groupIdx)
{
	bool flip = false;
	float zn = 1; float zf = 50;
	int rays_pp = 1;
	const Camera* active_camera = &camera;

	float4x4 inv_view = glm::inverse(active_camera->view());
	float4x4 inv_proj = glm::inverse(active_camera->projection());
	float3 o = active_camera->eye;
		
    int i_start = (int)floor((float)groupIdx/cachedWorkThreadN_ * size_.y);
    int i_end = (int)floor((float)(groupIdx+1)/cachedWorkThreadN_ * size_.y);

    int samples_n = 0;
	bool clear = clearSignal_[groupIdx];
	for(int i = 0; i < size_.y; i++)
	{
		if((i % cachedWorkThreadN_) != groupIdx) continue;
		for(int j = 0; j < size_.x; j++)
		{
            bool debug = false;
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
			
			int desiredBounces = 15;
			includeDirect_ = true;
			debugExclusiveBounce_ = 3;
			if(1)
			{
				maxDepth_ = desiredBounces;
				processSampleToCompletionMis(rand, &sample);
			}
			else if(0)
			{
				maxDepth_ = desiredBounces;
				processSampleToCompletion(rand, &sample);
			}
			else 
			{
				maxDepth_ = desiredBounces + 1;
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
RenderCore::RenderCore() : size_(-1, -1), background(0), stopSignal_(false), cachedWorkThreadN_(-1)
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
	Rand rand;
	int iteration = 0;
	auto start = boost::chrono::system_clock::now();
	while(!stopSignal_)
		//&& boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::system_clock::now() - start).count() < 5000)
	{
		step(rand, groupIdx);
		iteration++;
		cout << "group: " << groupIdx << " iteration:" << iteration << endl;
	}
}