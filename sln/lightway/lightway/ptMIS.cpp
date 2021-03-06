#include "stdafx.h"
#include "RenderCore.h"
#include "bxdf.h"
#include "shapes.h"
float powerHeuristic(float pa, float pb)
{
	float pa2 = pa * pa;
	float pb2 = pb * pb;
	return pa2 / (pa2 + pb2);
}
float balanceHeuristic(float pa, float pb)
{
	return pa / (pa + pb);
}
float3 directLightSampleLight(Rand& rand, const RTScene& scene, 
	const Intersection& pt, 
	const float3& wo,
	const ShadingCS& shadingCS,
	int* lightId)
{
	auto & light = scene.accl->lights[0];
	*lightId = INVALID_LIGHT_ID;
	float3 lightPos;
	float3 wiDirectWorld;
	float lightPdf;
	float lightT;
	//sample the light
	light.sample(rand, pt.position, &lightPos, &wiDirectWorld, &lightPdf, &lightT);

	if(!facing(pt.position, pt.normal, lightPos, light.normal)) return float3(0);
	//see if we're occluded
	Ray shadowRay(pt.position, wiDirectWorld);
	//we cannot ignore the light here, as we need to differentiate b/t hit empty space
	//and hit light
	IntersectionQuery shadowQuery(shadowRay);	

	Intersection sceneIsect;
	Intersection lightIsect;
	intersectScene(scene, shadowQuery, &sceneIsect, &lightIsect);
	bool hitLight = hitLightFirst(sceneIsect, lightIsect) && (lightIsect.lightId == light.id);
	if(!(hitLight && facing(pt.position, pt.normal, lightIsect.position, lightIsect.normal)))
	{
		//we missed the light, or we're in shadow, or we're not facing it
		return float3(0, 0, 0);
	}
	float3 wiDirect = shadingCS.local(wiDirectWorld);
	float brdfPdf = pt.material->pdf(wiDirect, wo);
	float3 brdfEval = pt.material->eval(wiDirect, wo);;
	float3 cosTheta = float3(dot(pt.normal, wiDirectWorld));
	float3 le = light.material.emission;
	*lightId = lightIsect.lightId;
	//balanced heuristic - fPdf / (fPdf + gPdf) * fPdf simplifies to 1/(fPdf + gPdf)
	return le * brdfEval * cosTheta * float3(powerHeuristic(lightPdf, brdfPdf)) / lightPdf;
}
/*
preconditions
- the scene/light points are visible and facing 
*/
float3 directLightSampleBrdf(const RectangularAreaLight& light, 
	float distance, const float3& wiWorld, const float& cosTheta, float brdfPdf, const float3& brdfEval)
{
	float lightPdf = light.pdf(wiWorld, distance);
	float3 le = light.material.emission;
	return le * brdfEval * cosTheta * float3(powerHeuristic(brdfPdf, lightPdf)) / brdfPdf;
}

void ptMISRun( const RTScene& scene, int bounces, Rand& rand, Sample* sample, bool useShadingNormals )
{
	float3 throughput(1);

	Ray woWorldRay;
	Intersection isect;
	const float rrStartDepth = glm::min(glm::max(bounces / 2, 3), 5);
	{
		IntersectionQuery isectQuery(sample->ray);
		Intersection lightIsect;
		intersectScene(scene, isectQuery, &isect, &lightIsect);
		if(hitLightFirst(isect, lightIsect))
		{
			sample->radiance = lightIsect.material->emission;
			return;
		}
		woWorldRay = sample->ray;
	}
	for(int vertIdx = 0; vertIdx < bounces; vertIdx++)
	{		
		if(!isect.hit)
		{
			sample->radiance += throughput * scene.background;
			return;
		}

		ShadingCS sceneIsectShadingCS(isect.normal);
		float3 wo = sceneIsectShadingCS.local(-woWorldRay.dir);
		
		int lightId = INVALID_LIGHT_ID;
		bool bounceIsDelta = isect.material->isDelta();
		if(!bounceIsDelta)
		{
			sample->radiance += throughput * 
				directLightSampleLight(rand, scene, isect, wo, sceneIsectShadingCS, &lightId);
		}

		//sample the brdf
		float3 weight;
		float3 wiIndirect;

		isect.material->sample(wo, float2(rand.next01(), rand.next01()), &wiIndirect, &weight);
		if(isBlack(weight))
		{
			return;
		}
		float brdfPdf = isect.material->pdf(wiIndirect, wo);
		float3 brdfEval = isect.material->eval(wiIndirect, wo);
		float3 wiWorldIndirect = sceneIsectShadingCS.world(wiIndirect);


		//don't mul. throughput by weight just yet... we need to add light
		//intersect with the scene
		Ray nextWoWorldRay(isect.position, wiWorldIndirect);
		IntersectionQuery nextSceneIsectQuery(nextWoWorldRay);

		Intersection nextSceneIsect;
		Intersection lightIsect;
		intersectScene(scene, nextSceneIsectQuery, &nextSceneIsect, &lightIsect);
		if(hitLightFirst(nextSceneIsect, lightIsect))
		{
			float cosThetaWI = dot(wiWorldIndirect, isect.normal);
			auto & light = *scene.light(lightIsect.lightId);
			if(!bounceIsDelta)
			{
				if(lightId == lightIsect.lightId)
				{
					sample->radiance += throughput * 
						directLightSampleBrdf(light, lightIsect.t, 
						wiWorldIndirect, cosThetaWI, brdfPdf, brdfEval);
				}
			}
			else
			{
				//no cos for specular
				sample->radiance += throughput * light.material.emission * brdfEval; 
			}
			break; //eyes block eye rays
		}
		//for collision precision
		if(dot(-wiWorldIndirect, nextSceneIsect.normal) <= 0) break;
		//we can't use this weight for directLightSampleBrdf, b/c it includes the brdfPdf
		throughput *= weight;
		isect = nextSceneIsect;
		woWorldRay = nextWoWorldRay;
		/*
		if(vertIdx > rrStartDepth)
		{
			float survivalProb = glm::min(luminance(throughput), 0.5f);
			if(rand.next01() > survivalProb) break;
			else throughput /= survivalProb;
		}
		*/
	}
}
//russian roulette code
/*
*/