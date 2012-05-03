#include "stdafx.h"
#include "RenderCore.h"
#include "bxdf.h"
#include "shapes.h"
#include <glm/ext.hpp>
#include <iostream>

bool closest_intersect_ray_scene(const RTScene& scene, const IntersectionQuery& query, Intersection* intersection)
{
	Intersection scene_intersection;
	bool scene_hit = scene.accl->intersect(query, &scene_intersection);

	Intersection light_intersection;
	//bool light_hit = scene.area_lights[0].intersect(query.ray, &light_intersection, query.flipRay);
	bool light_hit = scene.area_lights[0].intersect(query, &light_intersection);
	//make sure the light hit is valid
	light_hit = light_hit && query.isValid(light_intersection);
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
}
void axisConversions(const float3& normal, float3x3* zUpToWorld, float3x3* worldToZUp)
{
	//hopefully we can collapse this..
	if(normal.x == 0 && normal.y == 0 && normal.z == 1)
	{
		//do nothing
	}
	else if(normal.x == 0 && normal.y == 0 && normal.z == -1)	
	{
		float3 rotaxis(0, 1, 0);
		float rotangle = PI;
		*zUpToWorld = float3x3(glm::rotate(glm::degrees(rotangle), rotaxis));
	}
	else
	{					
		float3 rotaxis = cross(float3(0, 0, 1), normal);
		float rotangle = acos(dot(normal, float3(0, 0, 1)));
		*zUpToWorld = float3x3(glm::rotate(glm::degrees(rotangle), rotaxis));
	}
	*worldToZUp = glm::transpose(*zUpToWorld);	
}
//always samples according to the light. 
//the MIS version combines light sampling with brdf sampling
float3 directLight(Rand& rand, const RTScene& scene, const Intersection& pt, const float3x3& worldToZUp, const float3& wo)
{
	float3 lightPos;
	float3 wiDirectWorld;
	float lightPdf;	
	float lightT;

	scene.area_lights[0].sample(rand, pt.position, &lightPos, &wiDirectWorld, &lightPdf, &lightT);

	Ray shadow_ray(pt.position, wiDirectWorld);
	IntersectionQuery shadowQuery(shadow_ray, true);
	Intersection occluderIntersection;
	bool occluderHit = scene.accl->intersect(shadowQuery, &occluderIntersection);
		
	//light has to be facing the surface
	//-wiDirectWorld . lightNormal > 0
	//wiDirectWorld . normal > 0
	bool lightFacingSurface = 
		(dot(-wiDirectWorld, scene.area_lights[0].normal) > 0) &&
		(dot(wiDirectWorld, pt.normal) > 0);
	bool unoccluded = !occluderHit || (lightT < occluderIntersection.t);
	if(unoccluded && lightFacingSurface)
	{		
		float3 wiDirect = worldToZUp * wiDirectWorld;
		float3 brdfEval = pt.material->fresnelBlend.eval(wiDirect, wo);
		//float3 brdfEval = closest.material->fresnelBlend.lambertBrdf.eval();
		float3 ndotl = float3(dot(pt.normal, wiDirectWorld) );
		lwassert_greater(ndotl.x, 0);
		float3 le = scene.area_lights[0].material->emission;
			
		float3 direct = le * brdfEval * ndotl / lightPdf;
		return direct;
	}	
	return float3(0);
}

float3 directLightMis(Rand& rand, const RTScene& scene, const Intersection& pt, const float3x3& worldToZUp, 
	const float3x3& zUpWoWorld, const float3& wo)
{
	bool sampleLight = rand.next01() > 0.5;
	auto & light = scene.area_lights[0];
	float lightPdf;
	float brdfPdf;
	float3 wiDirectWorld;
	float lightT;
	float3 wiDirect; //only initally valid if we sampled the brdf
	if(sampleLight)
	{
		//sample light
		float3 lightPos;
		light.sample(rand, pt.position, &lightPos, &wiDirectWorld, &lightPdf, &lightT);
	}
	else
	{
		//sample brdf
		float3 weight;
		pt.material->fresnelBlend.sample(wo, float2(rand.next01(), rand.next01()), &wiDirect, &weight);
		brdfPdf = pt.material->fresnelBlend.pdf(wiDirect, wo);
		wiDirectWorld = zUpWoWorld * wiDirect;
	}
	//wiDirectWorld is now set

	//see if the light is occluded
	//first, intersect with the world
	Ray shadowRay(pt.position, wiDirectWorld);
	IntersectionQuery shadowQuery(shadowRay, true);	
	Intersection occluderIntersection;
	bool hitOccluder = scene.accl->intersect(shadowQuery, &occluderIntersection);	
	bool hitLight = sampleLight; //if we sampled the light, we know we hit it!
	if(!sampleLight)
	{
		//only intersect with light if we brdf sampled (didn't light sample)
		shadowQuery.flipRay = false; //flip back (originally flipped)
		Intersection lightIntersection;
		hitLight = light.intersect(shadowQuery, &lightIntersection);		
		lightT = lightIntersection.t;
	}
	if(!hitLight) 
	{
		return float3(0);
	}
	//we know we hit the light now...
	
	//lightT is now set
	bool unoccluded = !hitOccluder || lightT < occluderIntersection.t;
	bool lightFacingSurface = 
		(dot(wiDirectWorld, -light.normal) > 0) && //light intersection should skip this... but??
		(dot(wiDirectWorld, pt.normal) > 0); //can happen if we sample light and it's partially behind us
	if(!(unoccluded && lightFacingSurface))
	{
		//we missed the light, or we're in shadow, or we're not facing it
		return float3(0);
	}
	//if unoccluded, combine the two
	if(sampleLight)
	{		
		//get brdf pdf
		wiDirect = worldToZUp * wiDirectWorld;
		brdfPdf = pt.material->fresnelBlend.pdf(wiDirect, wo);
	}
	else
	{
		//get light pdf
		lightPdf = light.pdf(wiDirectWorld, lightT);
	}
	//wiDirect is now all set
	//eval the brdf
	float3 brdfEval = pt.material->fresnelBlend.eval(wiDirect, wo);
	float3 cosTheta = float3(dot(pt.normal, wiDirectWorld));
	float3 le = light.material->emission;
	//balanced heuristic - fPdf / (fPdf + gPdf) * fPdf simplifies to 1/(fPdf + gPdf)
	lwassert_validfloat(lightPdf);
	lwassert_validfloat(brdfPdf);
	lwassert_greater(lightPdf, 0);
	lwassert_greater(brdfPdf, 0);
	return float3(2) * le * brdfEval * cosTheta / (lightPdf + brdfPdf);
}
const int RANDOM_TERMINATION_DEPTH = 3;
const int RT_MAX_DEPTH = 8;
void RenderCore::processSample(Rand& rand, Sample* sample)
{
	auto & sd = sampleDebugger_;
	if(sample->depth > RT_MAX_DEPTH) 
	{
		sample->finished = true; 
		return;
	}
	
	sd.shr.record(sample->xy, sample->depth, "Throughput Start", sample->throughput);
	
	if(sample->depth > RANDOM_TERMINATION_DEPTH)
	{
		float survival = glm::min(luminance(sample->throughput), 0.5f);
		if(rand.next01() > survival)
		{
			sample->finished = true;
			return;
		}
		sample->throughput /= survival;
	}
	
	
	Intersection closest;
	IntersectionQuery bounceQuery(sample->ray, false);
	bool hit = closest_intersect_ray_scene(*scene, bounceQuery, &closest);
	if(!hit)
	{			
		sd.shr.record(sample->xy, sample->depth, "Missed", float3(1));
		sample->radiance += sample->throughput * background;
		sample->finished = true;
		return;
	}
	
	sd.shr.record(sample->xy, sample->depth, "normal", closest.normal);
	if(sample->depth == 0)
	{
		float3 emission = sample->throughput * closest.material->emission;
		sample->radiance += emission;
	}
		
	float3x3 zUpToWorld;
	float3x3 worldToZUp;
	axisConversions(closest.normal, &zUpToWorld, &worldToZUp);
		
	float3 wo = worldToZUp * -sample->ray.dir;
	//update radiance with direct light
	//sample->radiance += sample->throughput * directLight(rand, *scene, closest, worldToZUp, wo);
	sample->radiance += sample->throughput * directLightMis(rand, *scene, closest, worldToZUp, zUpToWorld, wo);
	//update ray/throughput with brdf	
	{
		float3 weight;			
		float3 wiIndirect;
		//closest.material->fresnelBlend.lambertBrdf.sample(float2(rand.next01(), rand.next01()),
		//	&wiIndirect, &weight);
		closest.material->fresnelBlend.sample(wo, float2(rand.next01(), rand.next01()), &wiIndirect, &weight);
		//closest.material->fresnelBlend.lambertBrdf.sample(float2(rand.next01(), rand.next01()), &wiIndirect, &weight);
		
		float3 wiWorldIndirect = zUpToWorld * wiIndirect;
		sample->throughput *= weight;
		sd.shr.record(sample->xy, sample->depth, "wi Indirect", wiIndirect);
		sd.shr.record(sample->xy, sample->depth, "wiWorld Indirect", wiWorldIndirect);
		sd.shr.record(sample->xy, sample->depth, "Indirect Weight", weight);
		sd.shr.record(sample->xy, sample->depth, "Throughput End", sample->throughput);

		const float requiredMinThroughput = 0.001f;
		if(glm::all(glm::lessThan(sample->throughput, float3(requiredMinThroughput))))
		{
			sample->finished = true;
			sd.shr.record(sample->xy, sample->depth, "Terminated", float3(1));
		}
		else
		{				
			sample->ray = Ray(closest.position + 0.001f * wiWorldIndirect, wiWorldIndirect);
			sample->depth++;
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
			
			while(!sample.finished)
			{				
				processSample(rand, &sample);
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

void RenderCore::workThread(int groupIdx)
{
	Rand rand;
	int iteration = 0;
	while(!stopSignal_)
	{
		step(rand, groupIdx);
		iteration++;
		cout << "group: " << groupIdx << " iteration:" << iteration << endl;
	}
}