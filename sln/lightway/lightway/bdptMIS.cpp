#include "stdafx.h"
#include "RenderCore.h"
#include "bxdf.h"
#include "shapes.h"
namespace bdptMIS
{
	struct PathVertexMIS
	{
		PathVertexMIS() { }
		PathVertexMIS(const float3& normal, 
			const float3& woWorld, 
			const float3& position, 
			const Material* material, 
			const float3& a,
			const float3& wo = float3(-10000000)) 
			: normal(normal), woWorld(woWorld), position(position), material(material), throughput(a), wo(wo)
		{

		}
		float3 normal;
		float3 woWorld;
		float3 position;
		const Material* material;
		float3 throughput;
		float gWo; //geometry term

		float pWi; //probability for i-1, i, i+1 path (does not exist at first/last verts)
		float pWo; //probability for i+1, i, i-1 path (does not exist at first/last verts)
		float pA; //probability for light

		float3 wi; //does not exist at light vertex, last vertex
		float3 wo; //does not exist at light vertex
		bool isDelta() const
		{ 
			//not delta if it's a light...
			return isBlack(material->emission) && material->isDelta(); 
		}
	};
	void computePathWoGeometry(int numVerts, PathVertexMIS vertices[])
	{
		//compute geometry term
		for(int i = 1; i < numVerts; i++)
		{
			const PathVertexMIS & curr = vertices[i];
			const PathVertexMIS & prev = vertices[i-1];
			float3 woWorld = normalize(curr.position - prev.position);
			float d2 = glm::distance2(curr.position, prev.position);
			vertices[i].gWo = dot(woWorld, curr.normal) * dot(-woWorld, prev.normal) / d2;
		}
	}
	int createPath(const RTScene& scene, 
		Rand& rand, 
		int maxVerts,
		const float3& initialAlpha, 
		const Ray& initialRay,
		bool includeLightIntersections,
		PathVertexMIS vertices[])
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
			if(hitNothing(isect, lightIsect)) break;


			if(hitLightFirst(isect, lightIsect))
			{				
				if(includeLightIntersections)
				{
					vertices[i] = PathVertexMIS(lightIsect.normal, woWorld, lightIsect.position, lightIsect.material, 
						alpha); //light isect don't need wi/wo
					numVerts++;
					break;
				}
				else
				{
					break;
				}
			}
			numVerts++;

			ShadingCS isectShadingCS(isect.normal);
			float3 wo = isectShadingCS.local(woWorld);
			vertices[i] = PathVertexMIS(isect.normal, woWorld, isect.position, isect.material, alpha, wo);
			
			if(i == (maxVerts - 1)) break;

			float3 wi;
			float3 weight;
			isect.material->sample(wo, rand.next01f2(), &wi, &weight);
			
			vertices[i].pWi = isect.material->pdf(wi, wo);
			if(i > 0) vertices[i].pWo = isect.material->pdf(wo, wi);

			if(isBlack(weight))
			{
				break;
			}
			vertices[i].wi = wi;
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
		PathVertexMIS vertices[])
	{
		int numVerts = createPath(scene, rand, maxVerts, float3(1), initialRay, true, vertices);
		if(numVerts > 0) vertices[0].pWo = 1;
	}
	int createLightPath(const RTScene& scene, 
		Rand& rand, 
		int maxVerts,
		PathVertexMIS vertices[])
	{
		const auto & light = *scene.light(0);
		ShadingCS lightPosShadingCS(light.normal);
		const float3 lightPos = light.samplePoint(rand);
		const float3 woWorldLight = lightPosShadingCS.world(sampleHemisphere(rand));
		Ray lightRay(lightPos, woWorldLight);
		vertices[0] = PathVertexMIS(light.normal, float3(0), lightPos, &light.material,
			light.material.emission * light.area());
		float3 lightPdf = float3(1.f / (2 * PI * light.area()));
		float3 alpha = light.material.emission * dot(woWorldLight, light.normal) / lightPdf;
		vertices[0].pA = 1.f/light.area();

		return 1 + createPath(scene, rand, maxVerts - 1, alpha, lightRay, false, &vertices[1]);
	}

	float3 directLight(
		Rand& rand, 
		const RTScene& scene, 
		const PathVertexMIS& vertex,
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
	PathVertexMIS lpVerts[16];
	PathVertexMIS epVerts[16];

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
				ShadingCS lvShadingCS(lv.normal);
				float3 lvWi = lvShadingCS.local(lvWiWorld);
				float3 lvWo = lvShadingCS.local(lv.woWorld);
				if(lpvIdx == 0)
				{
					lvBrdfEval = float3(1);
				}
				else
				{				
					lvBrdfEval = lv.material->eval(lvWi, lvWo);
				}

				float g = dot(evWiWorld, ev.normal) 
					* dot(lvWiWorld, lv.normal) / glm::distance2(lv.position, ev.position);

				//compute probabilities
				{
					float pSum = 0;
					int epVertCount = 2 + epvIdx;
					int lpVertCount = 1 + lpvIdx;	

					float pLP = ev.gWo * ev.pWo / (lv.material->pdf(lvWi, lvWo) * g);
					float pEP = 1/pLP;

					for(int i = lpvIdx - 1; i > 0; i--) //start at i+2
					{
						int vertIdx = lpvIdx - i + epVertCount;

						auto & prevVert = lpVerts[i-1];
						auto & currVert = lpVerts[i];
						auto & nextVert = lpVerts[i+1];
						float pRatio = prevVert.pWi * currVert.gWo / (nextVert.pWo * nextVert.gWo);

						p[vertIdx] = p[vertIdx - 1] * pRatio;
					}
					for(int i = epvIdx - 2; i > 0; i--) //start at i-2
					{
						int vertIdx = i + 1;
						auto & prevVert = lpVerts[i-1];
						auto & currVert = lpVerts[i];
						auto & nextVert = lpVerts[i+1];
						float pRatio = (nextVert.pWo * nextVert.gWo) / (prevVert.pWi * currVert.gWo);

						p[vertIdx] = p[vertIdx + 1] * pRatio;
					}
					//take into account probabilities at endpoints
				}

				float3 lpT = lvBrdfEval * lv.throughput;
				float3 epT = evBrdfEval * ev.throughput;
				float3 T = lpT * epT * g;

				sample->radiance += bdptWeight * T;
			}
		}
	}
}
