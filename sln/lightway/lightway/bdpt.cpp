#include "stdafx.h"
#include "RenderCore.h"
#include "bxdf.h"
#include "shapes.h"

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
int createLightPath(const RTScene& scene, Rand& rand, Intersection isects[], float3 T[])
{
	const auto & light = *scene.light(0);
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
		intersectScene(scene, query, &isect, nullptr);
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


void bdptRun(const RTScene& scene, int bounces, Rand& rand, Sample* sample)
{
	//const float rrStartDepth = glm::min(glm::max(maxDepth_ / 2, 3), 5);
	Intersection lightPathIsect[MAX_LIGHT_PATH_DEPTH + 1];
	float3 lightPathThroughputs[MAX_LIGHT_PATH_DEPTH + 1]; //the throughput from vert i -> 0
	int lightPathIsectsN = createLightPath(scene, rand, lightPathIsect, lightPathThroughputs);
	float3 throughput(1);
	//trace the eye ray 
	Ray sceneRay = sample->ray;
	for(int depth = 0; depth < (bounces + 1); depth++)
	{
		Intersection sceneIsect;
		Intersection lightisect;
		IntersectionQuery sceneIsectQuery(sceneRay);
		intersectScene(scene, sceneIsectQuery, &sceneIsect, &lightisect);
		if(hitLightFirst(sceneIsect, lightisect))
		{
			//there's NO shadow ray here! (so not depth + 1 + 1)
			int pathLength = depth + 1;

			float bdptWeight = (1.f / (pathLength));
			sample->radiance += bdptWeight * throughput * lightisect.material->emission;

		}
		if(!sceneIsect.hit) return; //we didn't hit scene
		//we only came here for the direct connection... 
		if(depth == bounces) break;
		
		ShadingCS evShadingCS(sceneIsect.normal);
		float3 woEV = evShadingCS.local(-sceneRay.dir);
		for(int lightPathIsectIdx = 0; lightPathIsectIdx < lightPathIsectsN; lightPathIsectIdx++)			
		{
			//there's a shadow ray here! (so +1)
			int pathLength = (lightPathIsectIdx) + (depth + 1) + 1;
			//todo: make the following depend on Russian Roulette data
			//we lose a variability b/c our first eye path is not removable

			float bidirectionalWeight = 1.f / (pathLength);
			
			
			if(pathLength > (bounces + 1)) continue;

			if(visibleAndFacing(lightPathIsect[lightPathIsectIdx], sceneIsect, scene))
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
	}
}


//russian roulette
/*
if(depth > rrStartDepth)
{
	float survivalProb = glm::min(luminance(throughput), 0.5f);
	if(rand.next01() > survivalProb) break;
	else throughput /= survivalProb;//glm::min(survivalProb * 2.f, 1.f);
}
*/