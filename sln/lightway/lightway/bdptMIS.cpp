#include "stdafx.h"
#include "RenderCore.h"
#include "bxdf.h"
#include "shapes.h"
namespace bdptMIS
{
	struct PathVertex
	{
		PathVertex() { }
		PathVertex(const float3& normal, 
			const float3& woWorld, 
			const float3& position, 
			const Material* material, 
			const float3& a) 
			: normal(normal), woWorld(woWorld), position(position), material(material), throughput(a)
		{

		}
		float3 normal;
		float3 woWorld;
		float3 position;
		const Material* material;
		float3 throughput;
		bool isDelta() const
		{ 
			//not delta if it's a light...
			return isBlack(material->emission) && material->isDelta(); 
		}
	};

	int createPath(const RTScene& scene, 
		Rand& rand, 
		int maxVerts,
		const float3& initialAlpha, 
		const Ray& initialRay,
		bool includeLightIntersections,
		PathVertex vertices[])
	{
		int numVerts = 0;
		Ray ray = initialRay;
		float3 alpha = initialAlpha;
		for(int i = 0; i < maxVerts; i++)
		{		
			float3 woWorld = -ray.dir;
			IntersectionQuery query(ray);
			Intersection isect;
			Intersection lightIsect;

			intersectScene(scene, query, &isect, &lightIsect);
			if(hitLightFirst(isect, lightIsect))
			{				
				if(includeLightIntersections)
				{
					vertices[i] = PathVertex(lightIsect.normal, woWorld, lightIsect.position, lightIsect.material, 
						alpha);
					return numVerts + 1;
				}
				else
				{
					return numVerts;
				}
			}


			if(!isect.hit) break;
			else numVerts++;

			vertices[i] = PathVertex(isect.normal, woWorld, isect.position, isect.material, alpha);

			if(i == (maxVerts - 1)) break;

			ShadingCS isectShadingCS(isect.normal);
			float3 wi;
			float3 weight;
			isect.material->sample(isectShadingCS.local(woWorld), rand.next01f2(), &wi, &weight);
			if(isBlack(weight))
			{
				break;
			}
			float3 wiWorld = isectShadingCS.world(wi);		
			ray.origin = isect.position;		
			ray.dir = wiWorld;
			alpha *= weight;
		}
		return numVerts;
	}
	int createEyePath(const RTScene& scene, 
		Rand& rand, 
		int maxVerts,
		const Ray& initialRay,
		PathVertex vertices[])
	{
		return createPath(scene, rand, maxVerts, float3(1), initialRay, true, vertices);
	}
	int createLightPath(const RTScene& scene, 
		Rand& rand, 
		int maxVerts,
		PathVertex vertices[])
	{
		const auto & light = *scene.light(0);
		ShadingCS lightPosShadingCS(light.normal);
		const float3 lightPos = light.samplePoint(rand);
		const float3 woWorldLight = lightPosShadingCS.world(sampleHemisphere(rand));
		Ray lightRay(lightPos, woWorldLight);
		vertices[0] = PathVertex(light.normal, float3(0), lightPos, &light.material,
			light.material.emission * light.area());
		float3 lightPdf = float3(1.f / (2 * PI * light.area()));
		float3 alpha = light.material.emission * dot(woWorldLight, light.normal) / lightPdf;
		//HACK: changed to intersect lights
		return 1 + createPath(scene, rand, maxVerts - 1, alpha, lightRay, false, &vertices[1]);
	}

	float3 directLight(
		Rand& rand, 
		const RTScene& scene, 
		const PathVertex& vertex,
		const ShadingCS& shadingCS)
	{
		//TODO: this does not work for multiple lights!
		auto & light = scene.accl->lights[0];
		float3 lightPos;
		float lightPdf; 
		//t is already incorporated into pdf
		float lightT;
		float3 wiDirectWorld;
		//sample the light
		light.sample(rand, vertex.position, &lightPos, &wiDirectWorld, &lightPdf, &lightT);

		//see if we're occluded
		Ray shadowRay(vertex.position, wiDirectWorld);
		//we cannot ignore the light here, as we need to differentiate b/t hit empty space
		//and hit light

		IntersectionQuery shadowQuery(shadowRay);	
		Intersection sceneIsect;
		Intersection lightIsect;
		intersectScene(scene, shadowQuery, &sceneIsect, &lightIsect);
		bool hitLight = hitLightFirst(sceneIsect, lightIsect) 
			&& lightIsect.lightId == light.id;
		//HACK
		//return float3(hitLight);//sceneIsect.hit ? sceneIsect.t : 0);
		if(hitLight && facing(vertex.position, vertex.normal, lightIsect.position, lightIsect.normal))
		{
			float3 wiDirect = shadingCS.local(wiDirectWorld);
			float3 brdfEval = vertex.material->eval(wiDirect, shadingCS.local(vertex.woWorld));
			float3 cosTheta = float3(dot(vertex.normal, wiDirectWorld));
			float3 le = light.material.emission;
			return le * brdfEval * cosTheta / lightPdf;
		}
		return float3(0);
	}

}
using namespace bdptMIS;
void bdptMisRun(const RTScene& scene, int bounces, Rand& rand, Sample* sample)
{	
	PathVertex lpVerts[16];
	PathVertex epVerts[16];

	//we disregard direct light->eye connection
	//we include direct eye->light connection
	int maxEyeVertices = bounces + 1; //explicit vertices (the one at the eye is implicit)
	int maxLightVertices = bounces;
	int numLPV = createLightPath(scene, rand, maxLightVertices, lpVerts);
	//starts at 2 vertices
	int numEPV = createEyePath(scene, rand, maxEyeVertices, sample->ray, epVerts);
	//maps path length to deltas
	//lowest = 1 vertices (1 eye, 0 light)

	//number of edges in the eye path that is non-specular
	//the direct connection edge does not count
	int epVariableNonSpecularEdges = 0; //initial excludes direct connection 
	for(int epvIdx = 0; epvIdx < numEPV; epvIdx++)
	{
		auto & ev = epVerts[epvIdx];

		if(ev.isDelta()) continue; //can't connect using shadow rays, or direct light

		//the current ev is already non delta
		if(epvIdx > 0 && !epVerts[epvIdx - 1].isDelta()) epVariableNonSpecularEdges++;

		if(!isBlack(ev.material->emission)) //we're the direct eye-light connection
		{
			int numTechniques = epVariableNonSpecularEdges + 1;
			float bdptWeight = 1.f / numTechniques;
			sample->radiance += bdptWeight * ev.throughput * ev.material->emission;			
			continue;
		}
		if(epvIdx == (maxEyeVertices - 1)) continue;

		ShadingCS evShadingCS(ev.normal);
		//direct lighting
		{
			int numTechniques = epVariableNonSpecularEdges + 2; //derived on paper...
			float bdptWeight = 1.f / numTechniques;
			sample->radiance += bdptWeight * ev.throughput * directLight(rand, scene, ev, evShadingCS);
		}
		
		float3 evWo = evShadingCS.local(ev.woWorld);
		//number of free non-specular light path edges (includes v0-v1)
		//disregard direct lighting (start at lpvIdx of 1)
		int lpVariableNonSpecularEdges = 0;
		for(int lpvIdx = 1; lpvIdx < numLPV; lpvIdx++) 
		{
			int lvCount = lpvIdx + 1;
			auto & lv = lpVerts[lpvIdx];
			if(lv.isDelta()) continue;
			
			//the current vertex is already non-specular
			//lpvIdx starts at one...
			if(!lpVerts[lpvIdx-1].isDelta()) lpVariableNonSpecularEdges++;

			int vCount = epvIdx + 2 + lpvIdx + 1;
			int currentBounces = vCount - 2;
			if(currentBounces > bounces) break;

			int numTechniques = lpVariableNonSpecularEdges + epVariableNonSpecularEdges + 2; //derived on paper
			float bdptWeight = 1.f / numTechniques;

			bool visible = visibleAndFacing(lv.position, lv.normal, ev.position, ev.normal, scene);
			if(visible)
			{
				ShadingCS lvShadingCS(lv.normal);
				
				const float3 ev2lv = normalize(lv.position - ev.position);
				float3 evWi = evShadingCS.local(ev2lv);
				const float3 evWiWorld = ev2lv;
				const float3 lvWiWorld = -ev2lv;
				float3 evBrdfEval = ev.material->eval(evWi, evWo);
				float3 lvBrdfEval;
				if(lpvIdx == 0)
				{
					lvBrdfEval = float3(1);
				}
				else
				{				
					ShadingCS lvShadingCS(lv.normal);
					float3 lvWi = lvShadingCS.local(lvWiWorld);
					float3 lvWo = lvShadingCS.local(lv.woWorld);
					lvBrdfEval = lv.material->eval(lvWi, lvWo);
				}

				float g = dot(evWiWorld, ev.normal) 
					* dot(lvWiWorld, lv.normal) / glm::distance2(lv.position, ev.position);

				float3 lpT = lvBrdfEval * lv.throughput;
				float3 epT = evBrdfEval * ev.throughput;
				float3 T = lpT * epT * g;

				sample->radiance += bdptWeight * T;
			}
		}
	}
}
