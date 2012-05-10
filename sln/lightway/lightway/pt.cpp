#include "stdafx.h"
#include "RenderCore.h"
#include "bxdf.h"
#include "shapes.h"

float3 directLight(Rand& rand, const RTScene& scene, 
	const Intersection& pt, 
	const float3& wo,
	const ShadingCS& shadingCS)
{
	auto & light = scene.accl->lights[0];
	float3 lightPos;
	float lightPdf; 
	//t is already incorporated into pdf
	float lightT;
	float3 wiDirectWorld;
	//sample the light
	light.sample(rand, pt.position, &lightPos, &wiDirectWorld, &lightPdf, &lightT);
	

	//see if we're occluded
	Ray shadowRay(pt.position, wiDirectWorld);
	//we cannot ignore the light here, as we need to differentiate b/t hit empty space
	//and hit light

	//HACK
	//shadowRay.dir = normalize(light.corners[0] - pt.position);

	IntersectionQuery shadowQuery(shadowRay);	
	Intersection sceneIsect;
	Intersection lightIsect;
	intersectScene(scene, shadowQuery, &sceneIsect, &lightIsect);
	bool hitLight = hitLightFirst(sceneIsect, lightIsect) 
		&& lightIsect.lightId == light.id;
	//HACK
	//return float3(hitLight);//sceneIsect.hit ? sceneIsect.t : 0);
	if(hitLight && facing(pt.position, pt.normal, lightIsect.position, lightIsect.normal))
	{
		float3 wiDirect = shadingCS.local(wiDirectWorld);
		float3 brdfEval = pt.material->eval(wiDirect, wo);
		float3 cosTheta = float3(dot(pt.normal, wiDirectWorld));
		float3 le = light.material.emission;
		return le * brdfEval * cosTheta / lightPdf;
	}
	return float3(0);
}
void ptRun(const RTScene& scene, int bounces, Rand& rand, Sample* sample)
{
	float3 throughput(1);
	Ray ray = sample->ray;
	bool lastBounceWasDelta = false;
	//an extra vertex for sampling specular (delta) reflection/refractions
	for(int vertIdx = 0; vertIdx < bounces + 1; vertIdx++)
	{		
		IntersectionQuery sceneIsectQuery(ray);

		Intersection sceneIsect;
		Intersection lightIsect;
		intersectScene(scene, sceneIsectQuery, &sceneIsect, &lightIsect);
		//if we happen to hit a light source, add Le and quit
		if(hitNothing(sceneIsect, lightIsect))
		{
			sample->radiance += throughput * scene.background;
			return;
		}	
		if(hitLightFirst(sceneIsect, lightIsect))
		{
			if(vertIdx == 0 || lastBounceWasDelta)
			{
				sample->radiance += throughput * lightIsect.material->emission;
			}
			break;
		}
		//the vertexIdx == maxVertices pass is only for perfectly specular stuff
		if(vertIdx == bounces) return;
		lastBounceWasDelta = false;
		if(!sceneIsect.hit) return;
		
		ShadingCS sceneIsectShadingCS(sceneIsect.normal);
		float3 wo = sceneIsectShadingCS.local(-ray.dir);

		if(!sceneIsect.material->isDelta())
		{
			sample->radiance += throughput * directLight(rand, scene, sceneIsect, wo, sceneIsectShadingCS);
		}
		else
		{
			lastBounceWasDelta = true;
		}
		float3 wiIndirect; 
		float3 indirectWeight;
		sceneIsect.material->sample(wo, rand.next01f2(), &wiIndirect, &indirectWeight);
		if(isBlack(indirectWeight))
		{
			return;
		}
		float3 wiWorld = sceneIsectShadingCS.world(wiIndirect);
		ray = Ray(sceneIsect.position, wiWorld);
		throughput *= indirectWeight;
	}
}