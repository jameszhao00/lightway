#include "stdafx.h"
#include "RenderCore.h"
#include "bxdf.h"
#include "shapes.h"
#include <glm/ext.hpp>
#include <iostream>

bool closest_intersect_ray_scene(const RTScene& scene, 
	const IntersectionQuery& query,
	Intersection* intersection)
{
	Intersection scene_intersection;
	//bool scene_hit = scene.accl->intersect(query, &scene_intersection);
	*intersection = scene.accl->intersect(query);
	return intersection->hit;

	/*
	Intersection light_intersection;
	bool light_hit = false;
	if(!ignoreLights)
	{
		light_hit = scene.area_lights[0].intersect(query, &light_intersection);
		//make sure the light hit is valid
		light_hit = light_hit && query.isValid(light_intersection);
	}
	if(scene_hit && light_hit)
	{
		*intersection = scene_intersection.t < light_intersection.t ? scene_intersection : light_intersection;
		return true;
	}
	else if(scene_hit)
	{
		*intersection = scene_intersection;
		return true;
	}
	else if(light_hit)
	{
		*intersection = light_intersection;
		return true;
	}
	return false;
	*/
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
	IntersectionQuery shadowQuery(shadowRay, false, true);	
	Intersection occluderIntersection;
	//bool hitOccluder = scene.accl->intersect(shadowQuery, &occluderIntersection);
	bool hitOccluder = closest_intersect_ray_scene(scene, shadowQuery, &occluderIntersection);
	
	bool unoccluded = !hitOccluder || lightT < occluderIntersection.t;
	bool lightFacingSurface = 
		(dot(wiDirectWorld, -light.normal) > 0) && //light intersection should skip this... but??
		(dot(wiDirectWorld, pt.normal) > 0); //can happen if we sample light and it's partially behind us
	if(!(unoccluded && lightFacingSurface))
	{
		//we missed the light, or we're in shadow, or we're not facing it
		return float3(0);
	}
	float3 wiDirect = worldToZUp * wiDirectWorld;
	//pdf the brdf
	//float brdfPdf = pt.material->fresnelBlend.pdf(wiDirect, wo);
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
const int RANDOM_TERMINATION_DEPTH = 4;
const int RT_MAX_DEPTH = 8;
void RenderCore::processSampleToCompletion(Rand& rand, Sample* sample)
{
	float3 throughput(1);

	Ray woWorldRay;
	Intersection isect;
	bool hitScene = false;
	//setup
	{
		//intersect with scene
		IntersectionQuery sceneIsectQuery(sample->ray, false, false);
		hitScene = closest_intersect_ray_scene(*scene, sceneIsectQuery, &isect);
		//if we happen to hit a light source, add Le and quit
		if(hitScene && validLightIdx(isect.lightIdx))
		{
			sample->radiance = isect.material->emission;
			return;
		}
		woWorldRay = sample->ray;
	}
	//path trace
	for(int depth = 0; depth < (RT_MAX_DEPTH + 1); depth++)
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
		//HACK: for testing
		//if(depth == 1)
		sample->radiance += throughput *
			directUsingLightSamplingMis(rand, *scene, isect, worldToZUp, zUpToWorld, wo, &lightIdx);		
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
		bool nextHitScene = closest_intersect_ray_scene(*scene, nextSceneIsectQuery, &nextIsect);
		//add direct light contribution from the BRDF sample direction (with MIS)
		if(nextHitScene && validLightIdx(nextIsect.lightIdx))
		{
			//if we hit a different light, quit... 
			//should probably change this later on
			if(lightIdx != nextIsect.lightIdx) return;
			else
			{		
				//add direct light contrib.
				//TODO: maybe we should mul. throughput by weight...
				//HACK: for testing
				//if(depth == 1)
				sample->radiance += throughput * 
					directUsingBrdfSamplingMis(*(scene->light(nextIsect.lightIdx)), nextIsect.t, 
					wiWorldIndirect, dot(wiWorldIndirect, isect.normal), brdfPdf, brdfEval);
			}
		}
		//mul. throughput by brdf weight
		throughput *= weight;
		if(depth > RANDOM_TERMINATION_DEPTH)
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
const int MAX_LIGHT_PATH_DEPTH = 5;

//visibility = facing each other, and not occluded
bool visibleAndFacing(const Intersection& isectA, const Intersection& isectB, bool ignoreLights, RTScene& scene)
{
	//make sure they're facing each other first 
	float3 dir = normalize(isectB.position - isectA.position);
	if(dot(dir, isectA.normal) < 0.00001) return false;
	if(dot(dir, isectB.normal) > -0.00001) return false;
	//now trace the ray...
	Ray ray(isectA.position, dir);
	IntersectionQuery query(ray, ignoreLights, false);
	Intersection actualIsect;
	float desiredT = glm::length(isectB.position - isectA.position);
	bool hit = closest_intersect_ray_scene(scene, query, &actualIsect);
	//see if we hit isect B
	if(!hit) return false;
	if(fabs(actualIsect.t - desiredT) > 0.0001) return false;
	return true;
}

//returns # of isects
//PRE CONDITION
//capacity of lightPathIsect/lightPathThroughputs = MAX_LIGHT_PATH_DEPTH + 1
//POST CONDITION
//lightPathIsect filled with MAX_LIGHT_PATH_DEPTH + 1 intersections
//lightPathThroughputs filled with MAX_LIGHT_PATH_DEPTH + 1 throughputs... each describing transfer from 0->i
//lightPathIsect[0] = starting point at light
//lightPathThroughputs[0] = light emission * light area
int createLightPath(const RTScene* scene, Rand& rand, Intersection* lightPathIsect, float3* lightPathThroughputs)
{
	//we start with light isect
	int numLightPathIsects = 1;
	
	//pick point on light
	auto & light = scene->area_lights[0];
	float3 lightPathPos = light.samplePoint(rand);
	//pick direction
	float3 lightPathWo = sampleHemisphere(rand);
		
	float3x3 zUpToWorld;
	float3x3 worldToZUp;
	axisConversions(light.normal, &zUpToWorld, &worldToZUp);
	float3 lightPathWoWorld = zUpToWorld * lightPathWo;
	//lightDirPDF does NOT BELONG here
	float3 lightDirPdf = INV_PI_V3;
	lightPathThroughputs[0] = light.material.emission * float3(light.area());
	float lightCosTheta = zup::cos_theta(lightPathWo);
	lightPathIsect[0].material = &light.material; 
	lightPathIsect[0].normal = light.normal;
	lightPathIsect[0].position = lightPathPos;
	//generate light path

	for(int depth = 1; depth < (MAX_LIGHT_PATH_DEPTH + 1); depth++)
	{			
		Ray lightPathRay(lightPathPos, lightPathWoWorld);
		IntersectionQuery lightPathQuery(lightPathRay, true, false);
		//check for intersection
		Intersection sceneIsect;
		bool lightPathHit = closest_intersect_ray_scene(*scene, lightPathQuery, &sceneIsect);
		//terminate if we hit nothing...
		if(!lightPathHit) break;
		else if(validLightIdx(sceneIsect.lightIdx)) break; //also break if we hit a light
		else numLightPathIsects++; //otherwise we like this intersection + record it!			
		if(depth == 1)
		{
			//we didn't know d before hand
			//so update it now
			//HACK: is multiplying by lightDirPdf correct here???
			lightPathThroughputs[1] = lightPathThroughputs[0] * lightCosTheta 
				/ (sceneIsect.t * sceneIsect.t * lightDirPdf);
		}
		if(depth < MAX_LIGHT_PATH_DEPTH) //if we're not terminating...
		{				
			axisConversions(sceneIsect.normal, &zUpToWorld, &worldToZUp);

			//chose new direction according to brdf
			float3 brdfWeight;
			//optimization: previous wo = this wi... no need to recalc every time
			float3 wi = worldToZUp * -lightPathWoWorld;
			float3 wo;
			//this is assuming the brdf is reciporical (e.g. we can plug in wi as wo...)
			//sceneIsect.material->fresnelBlend.sample(wi, float2(rand.next01(), rand.next01()), &wo, &brdfWeight);
			sceneIsect.material->fresnelBlend.lambertBrdf.sample(float2(rand.next01(), rand.next01()), &wo, &brdfWeight);
			
			//modify throughput by brdf weight
			//update lightPathWoWorld, lightPathPos
			lightPathWoWorld = zUpToWorld * wo;
							
			lightPathThroughputs[depth + 1] = brdfWeight * lightPathThroughputs[depth];
			
			lightPathPos = sceneIsect.position;
		}
		
		lightPathIsect[depth] = sceneIsect;
	}
	return numLightPathIsects;
}
void RenderCore::bidirectionallyProcessSampleToCompletion(Rand& rand, Sample* sample)
{
	Intersection lightPathIsect[MAX_LIGHT_PATH_DEPTH + 1];
	//for throughput[i], we don't include the weight at i
	float3 lightPathThroughputs[MAX_LIGHT_PATH_DEPTH + 1]; //the throughput from vert i -> 0
	int lightPathIsectsN = createLightPath(scene, rand, lightPathIsect, lightPathThroughputs);
	float3 throughput(1);
	//trace the eye ray 
	Ray sceneRay = sample->ray;
	//float debugTotalWeight[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	for(int depth = 0; depth < (RT_MAX_DEPTH + 1); depth++)
	{
		Intersection sceneIsect;
		IntersectionQuery sceneIsectQuery(sceneRay, false, false);
		bool hitScene = closest_intersect_ray_scene(*scene, sceneIsectQuery, &sceneIsect);
		//we hit nothing
		if(!hitScene)
		{
			return;
		}	
		//we hit a light
		if(validLightIdx(sceneIsect.lightIdx))
		{
			//TODO: this doesn't look right for bidirectional
			//TODO: this counts in bidirectional...
			if(depth == 0) sample->radiance += sceneIsect.material->emission;
			return;
		}
		float3x3 worldToZUp;
		float3x3 zUpToWorld;
		axisConversions(sceneIsect.normal, &zUpToWorld, &worldToZUp);
		float3 wo = worldToZUp * -sceneRay.dir;
		//for a intersection, test visibility with every lightPathIsect
		for(int lightPathIsectIdx = 0; lightPathIsectIdx < lightPathIsectsN; lightPathIsectIdx++)
		{
			int pathLength = (lightPathIsectIdx + 1) + (depth + 2) - 1;
			//todo: make the following depend on Russian Roulette data
			//we lose a variability b/c our first eye path is not removable
			float bidirectionalWeight = 1.f / (pathLength + 1);
			//debugTotalWeight[pathLength] += bidirectionalWeight;
			//hack:
			if(visibleAndFacing(lightPathIsect[lightPathIsectIdx], sceneIsect, true, *scene))
			{
				float3 eyePathIsectToLightPathIsectDir = 
					normalize(lightPathIsect[lightPathIsectIdx].position - sceneIsect.position);
				float3 eyeIsectBrdfEval;
				
				//eval brdf at sceneIsect
				{
					float3 wiEyeIsect = worldToZUp * eyePathIsectToLightPathIsectDir;
					const float3& woEyeIsect = wo;
					//eyeIsectBrdfEval = sceneIsect.material->fresnelBlend.eval(wiEyeIsect, woEyeIsect)
					eyeIsectBrdfEval = sceneIsect.material->fresnelBlend.lambertBrdf.eval()
						* zup::cos_theta(wiEyeIsect);
				}
				float3 lightIsectBrdfEval;
				//eval brdf at lightIsect		
				{
					float costheta = dot(lightPathIsect[lightPathIsectIdx].normal, -eyePathIsectToLightPathIsectDir);					
					float lightPathT2 = glm::distance2(lightPathIsect[lightPathIsectIdx].position, sceneIsect.position);
					if(lightPathIsectIdx == 0)
					{
						lightIsectBrdfEval = float3(costheta / (lightPathT2));
					}
					else
					{
						float3x3 worldToLightPathIsectZUp;
						float3x3 dummy;
						axisConversions(lightPathIsect[lightPathIsectIdx].normal, &dummy, &worldToLightPathIsectZUp);
						//TODO: optimization: we can cache wiLightIsect, worldToLightPathIsectZUp
						float3 wiLightIsect = worldToLightPathIsectZUp * normalize(lightPathIsect[lightPathIsectIdx - 1].position - 
							lightPathIsect[lightPathIsectIdx].position);
						float3 woLightIsect = worldToLightPathIsectZUp * -eyePathIsectToLightPathIsectDir;
						//lightIsectBrdfEval = float3(costheta) * lightPathIsect[lightPathIsectIdx].material->fresnelBlend.eval(wiLightIsect, woLightIsect);
						lightIsectBrdfEval = float3(costheta) * 
							lightPathIsect[lightPathIsectIdx].material->fresnelBlend.lambertBrdf.eval();
						
						//convert pdf
						{
							float ndotLightPathDir = wiLightIsect.z;
							float ndotEyePathDir = dot(lightPathIsect[lightPathIsectIdx].normal, 
								-eyePathIsectToLightPathIsectDir);
							float eyePathT2 = glm::distance2(sceneRay.origin, sceneIsect.position);
							//lightIsectBrdfEval *= ndotLightPathDir * eyePathT2 
							//	/ (ndotEyePathDir * lightPathT2);
						}
					}
				}
				float3 lightPathThroughput = lightPathThroughputs[lightPathIsectIdx] * lightIsectBrdfEval;
				float3 eyePathThroughput = throughput * eyeIsectBrdfEval;
				float3 bidirThroughput = lightPathThroughput * eyePathThroughput;
				//HACK: for testing
				//if(pathLength == 3 && lightPathIsectIdx == 1)
					sample->radiance += bidirectionalWeight * bidirThroughput;
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
	}
	//for debugging
	//for(int i = 0; i < sizeof(debugTotalWeight)/sizeof(float); i++)
	{
		//cout << "weight for path length " << i << " = " << debugTotalWeight[i] << endl;
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
			
			if(1) processSampleToCompletion(rand, &sample);
			else bidirectionallyProcessSampleToCompletion(rand, &sample);
			

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