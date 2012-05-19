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
			const float3& a)
			//,	const float3& wo = float3(-10000000)) 
			: normal(normal), 
			woWorld(woWorld), 
			position(position), 
			material(material), 
			throughput(a)//, 
			//wo(wo)
		{

		}
		float3 normal;
		float3 woWorld;
		float3 position;
		const Material* material;
		float3 throughput;
		float gWo; //geometry term

		float pWi; //probability for i-1, i, i+1 path (does not exist at first/last verts)
		float pA; //probability for light

		//float3 wi; //does not exist at light vertex, last vertex
		//float3 wo; //does not exist at light vertex
		bool isDelta() const
		{ 
			//not delta if it's a light...
			return isBlack(material->emission) && material->isDelta(); 
		}
	};
	struct ConnectedPath
	{
		ConnectedPath(const PathVertexMIS lpvs[], int numTotalLpv, const PathVertexMIS epvs[], int numTotalEpv) 
			: lpvs(lpvs), epvs(epvs), numTotalLpv(numTotalLpv), numTotalEpv(numTotalEpv)
		{
		}
		float pRatioForward(int i, int numPathVerts)
		{			
			int epvIdx = numPathVerts - i - 1;			
			LWASSERT(epvIdx < numTotalEpv);
			LWASSERT(epvIdx > 1);
			int lpvIdx = i;
			LWASSERT(lpvIdx < numTotalLpv);			
			float piPlus1 = (i == 0) ? lpvs[0].pA : (lpvs[i].gWo * lpvs[i-1].pWi);
			lwassert_validfloat(piPlus1);
			float pi = epvs[epvIdx].gWo * epvs[epvIdx - 1].pWi; //don't need to account for i == k
			lwassert_validfloat(pi);
			return piPlus1 / pi;
		}
		float pRatioBackward(int i, int numPathVerts)
		{
			LWASSERT(i > 0);
			LWASSERT(i <= numTotalLpv);
			LWASSERT(numTotalEpv >= (numPathVerts - i));

			float ratio = pRatioForward(i - 1, numPathVerts);
			lwassert_validfloat(ratio);
			if(ratio == 0) return 0;
			return 1.f / ratio;
		}
		float misWeight(int i, int numPathVerts)
		{
			//preconditions: numLV > 0, numEV > 0

			//check if pi can exist
			if(i > numTotalLpv) return 0;
			if((numPathVerts - i) > numTotalEpv) return 0;

			float invWeight = 1; //p[i]
			float p = 1; 

			for(int idx = i; idx < numPathVerts - 2; idx++)
			{
				if((idx+1) > numTotalLpv) break;

				p *= pRatioForward(idx, numPathVerts);
				lwassert_validfloat(p);

				LWASSERT_G(p, -0.000001);
				invWeight += p * p;
				LWASSERT_G(invWeight, -0.000001);
			}
			p = 1;
			for(int idx = i; idx > 0; idx--)	
			{
				if((numPathVerts - (idx-1)) > numTotalEpv) break;
				p *= pRatioBackward(idx, numPathVerts);
				lwassert_validfloat(p);

				LWASSERT_G(p, -0.000001);
				invWeight += p * p;
				LWASSERT_G(invWeight, -0.000001);

			}
			float weight = 1.f/invWeight;

			LWASSERT_G(weight, -0.000001);
			return weight;
		}
		void validate(int pathLength)
		{
			if(pathLength + 1 > (numTotalEpv + numTotalLpv)) return;
			float totalWeight = 0;
			int numVerts = pathLength + 1;
			if(numTotalEpv + numTotalLpv < numVerts) return; //can't possibly do this...
			for(int i = 0; i < numVerts - 1; i++)
			{
				float w = misWeight(i, numVerts);
				totalWeight += w;
			}
			if(totalWeight == 0)
			{
				//cout << "total weight: " << totalWeight << endl;
			}
			else if(fabs(totalWeight-1) > 0.000001f)
			{

				cout << "total weight: " << totalWeight << endl;
			}
		}
		int numTotalLpv;
		int numTotalEpv;
		const PathVertexMIS* const lpvs;
		const PathVertexMIS* const epvs;
	};
	void computePathGeometry(int numVerts, int startIdx, PathVertexMIS vertices[])
	{
		//compute geometry term
		for(int i = startIdx; i < numVerts; i++)
		{
			const PathVertexMIS & curr = vertices[i];
			const PathVertexMIS & prev = vertices[i-1];
			float3 wiWorldPrev = normalize(curr.position - prev.position);
			float d2 = glm::distance2(curr.position, prev.position);

			float num1 = dot(-wiWorldPrev, curr.normal);
			float num2 = dot(wiWorldPrev, prev.normal);
			//float num = dot(woWorld, curr.normal) * dot(woWorld, prev.normal);
			LWASSERT_G(num1, -0.000001);
			LWASSERT_G(num2, -0.000001);
			float num = (num1 * num2);
			float g = glm::saturate(num / d2);
			LWASSERT_G(g, -0.000001);
			vertices[i].gWo = g;
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

				LWASSERT(dot(woWorld, lightIsect.normal) > 0);
				if(includeLightIntersections)
				{
					vertices[i] = PathVertexMIS(lightIsect.normal, woWorld, lightIsect.position, lightIsect.material, 
						alpha); //light isect don't need wi/wo
					vertices[i].pWi = 1.f/1000000000.f;					
					numVerts++;
					break;
				}
				else
				{
					break;
				}
			}
			//handle intersection precision issue
			if(dot(woWorld, isect.normal) <=  0) break;

			numVerts++;

			ShadingCS isectShadingCS(isect.normal);
			float3 wo = isectShadingCS.local(woWorld);
			vertices[i] = PathVertexMIS(isect.normal, woWorld, isect.position, isect.material, alpha);
			
			if(i == (maxVerts - 1)) break;

			float3 wi;
			float3 weight;
			isect.material->sample(wo, rand.next01f2(), &wi, &weight);
			vertices[i].pWi = isect.material->pdf(wi, wo) / zup::cos_theta(wi);

			if(isBlack(weight))
			{
				break;
			}
			//vertices[i].wi = wi;
			float3 wiWorld = isectShadingCS.world(wi);
			LWASSERT_G(dot(wiWorld, isect.normal), -0.0001);
			ray.origin = isect.position;		
			ray.dir = wiWorld;
			alpha *= weight;
		}

		return numVerts;
	}
	int createEyePath(const RTScene& scene, 
		Rand& rand, 
		int maxVerts,
		float3 forward, 
		const Ray& initialRay,
		PathVertexMIS vertices[])
	{
		int numVerts = 1 + createPath(scene, rand, maxVerts, float3(1), initialRay, true, &vertices[1]);

		vertices[0].normal = float3(1.f/10000000);
		vertices[0].position = float3(1.f/10000000);
		vertices[0].pA = 10000000.f;
		vertices[0].pWi = 10000000.f;

		computePathGeometry(numVerts, 2, vertices);
		return numVerts;
	}
	int createLightPath(const RTScene& scene, 
		Rand& rand, 
		int maxVerts,
		PathVertexMIS vertices[])
	{
		const auto & light = *scene.light(0);
		ShadingCS lightPosShadingCS(light.normal);
		const float3 lightPos = light.samplePoint(rand);
		const float3 wiWorldLight = lightPosShadingCS.world(sampleHemisphere(rand));
		Ray lightRay(lightPos, wiWorldLight);
		vertices[0] = PathVertexMIS(light.normal, float3(0), lightPos, &light.material,
			light.material.emission * light.area());
		float3 lightRayPdf = float3(1.f / (2 * PI * light.area()));
		float3 alpha = light.material.emission * dot(wiWorldLight, light.normal) / lightRayPdf;
		vertices[0].pA = 1.f / light.area();
		vertices[0].pWi = (1.f / (2.f * PI * light.area())) /  dot(wiWorldLight, light.normal);

		int numVerts = 1 + createPath(scene, rand, maxVerts - 1, alpha, lightRay, false, &vertices[1]);
		
		computePathGeometry(numVerts, 1, vertices);
		return numVerts;
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
void bdptMisRun(const RTScene& scene, int bounces, const float3& forward, Rand& rand, Sample* sample)
{	
	//int debugPathLength = 2;
	
	PathVertexMIS lpVerts[16];
	PathVertexMIS epVerts[16];

	//we disregard direct light->eye connection
	//we include direct eye->light connection
	int maxEyeVertices = bounces + 2; //one at eye, one at the light hit
	int maxLightVertices = bounces;

	int numLPV = createLightPath(scene, rand, maxLightVertices, lpVerts);
	//starts at 2 vertices
	int numEPV = createEyePath(scene, rand, maxEyeVertices, forward, sample->ray, epVerts);

	ConnectedPath path(lpVerts, numLPV, epVerts, numEPV);
	//HACK: debugging
	for(int i = 2; i < bounces+2; i++)
	{
		path.validate(i);
	}
	float debugTotalWeights = 0;
	float debugExpectedDirectConnectionWeight = 0;
	float debugTotalUsedWeight = 0;

	for(int epvIdx = 1; epvIdx < numEPV; epvIdx++) //don't bother connecting the first eye vertex
	{
		auto & ev = epVerts[epvIdx];
		
		if(ev.isDelta()) continue; //can't connect using shadow rays, or direct light
		bool debugUsedDirect = false;
		if(!isBlack(ev.material->emission)) //we're the direct eye-light connection
		{
			int numPathVerts = epvIdx + 1;
			int currentBounces = numPathVerts - 2;
			if(currentBounces > bounces) continue;

			float misWeight = path.misWeight(0, numPathVerts);
			LWASSERT_G(misWeight, 0);
  			sample->radiance += misWeight * ev.throughput * ev.material->emission;	

			continue;			
		}
		debugTotalUsedWeight += path.misWeight(0, epvIdx + 1);
		//if(debugUsedDirect != (path.misWeight(0, epvIdx + 1) > 0)) cout << "direct - weight mismatch" << endl;
		if(epvIdx == (maxEyeVertices - 1)) continue;

		ShadingCS evShadingCS(ev.normal);
		//direct lighting
		/*
		{
			int numPathVerts = epvIdx + 1 + 1;
			sample->radiance += path.misWeight(1, numPathVerts) * ev.throughput * directLight(rand, scene, ev, evShadingCS);
		}
		*/
		
		float3 evWo = evShadingCS.local(ev.woWorld);		
		for(int lpvIdx = 0; lpvIdx < numLPV; lpvIdx++) 
		{
			int numPathVertices = lpvIdx + 1 + epvIdx + 1;
			
			int lvCount = lpvIdx + 1;
			auto & lv = lpVerts[lpvIdx];
			if(lv.isDelta()) continue;
			

			int vCount = epvIdx + 1 + lpvIdx + 1;
			int currentBounces = vCount - 2;
			if(currentBounces > bounces) break;

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


				float3 lpT = lvBrdfEval * lv.throughput;
				float3 epT = evBrdfEval * ev.throughput;
				float3 T = lpT * epT * g;
				float misWeight =  path.misWeight(lpvIdx + 1, numPathVertices);
				LWASSERT_G(misWeight, 0);
				sample->radiance += misWeight * T;
				
			}
		}	

	}
}
