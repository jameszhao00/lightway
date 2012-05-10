#include "stdafx.h"
#include "RenderCore.h"
#include "bxdf.h"
#include "shapes.h"

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
	bool isDelta() const { return material->isDelta(); }
};

//e.g. if MAX_LIGHT_PATH_DEPTH is 2
//eye isect[0]/throughput[0] ----throughput[1]---- isect[1] ----throughput[2]---- isect[2]
const int LIGHT_PATH_VERTS = 5;

//returns # of isects
//PRE CONDITION
//capacity of lightPathIsect/lightPathThroughputs = MAX_LIGHT_PATH_DEPTH + 1
//POST CONDITION
//lightPathIsect filled with MAX_LIGHT_PATH_DEPTH + 1 intersections
//lightPathThroughputs filled with MAX_LIGHT_PATH_DEPTH + 1 throughputs... each describing transfer from 0->i
//lightPathIsect[0] = starting point at light
//lightPathThroughputs[0] = light emission * light area

int createPath(const RTScene& scene, 
	Rand& rand, 
	int maxVerts,
	const float3& initialAlpha, 
	const Ray& initialRay,
	bool intersectLights,
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
		if(intersectLights)
		{
			intersectScene(scene, query, &isect, &lightIsect);
			if(hitLightFirst(isect, lightIsect))
			{				
				vertices[i] = PathVertex(lightIsect.normal, woWorld, lightIsect.position, lightIsect.material, 
					alpha);
				return numVerts + 1;
			}
		}
		else intersectScene(scene, query, &isect, nullptr);
		
		if(!isect.hit) break;
		else numVerts++;

		vertices[i] = PathVertex(isect.normal, woWorld, isect.position, isect.material, alpha);

		if(i == (maxVerts - 1)) break;

		ShadingCS isectShadingCS(isect.normal);
		float3 wi;
		float3 weight;
		isect.material->sample(isectShadingCS.local(woWorld), rand.next01f2(), &wi, &weight);
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
int createLightPath2(const RTScene& scene, 
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
	return 1 + createPath(scene, rand, maxVerts - 1, alpha, lightRay, true, &vertices[1]);
}
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
	for(int depth = 1; depth < (LIGHT_PATH_VERTS + 1); depth++)
	{		
		IntersectionQuery query(ray);
		Intersection isect;
		intersectScene(scene, query, &isect, nullptr);
		if(!isect.hit) break;
		else numIsects++;
		isects[depth] = isect;
		/*
		if(depth > 1)		
		{
			float pdf = 1 / (2*PI*dot(isects[depth-1].normal, ray.dir));
			ShadingCS prevIsectShadingCS(isects[depth-1].normal);
			float3 wi = prevIsectShadingCS.local(ray.dir);
			float3 wo = prevIsectShadingCS.local(normalize(isects[depth-2].position - isects[depth-1].position));
			float3 brdfEval = isects[depth-1].material->eval(wi, wo);
			T[depth] = T[depth - 1] * brdfEval / pdf;
		}
		*/
		if(depth == LIGHT_PATH_VERTS) break;

		ShadingCS isectShadingCS(isect.normal);
		float3 woWorld = normalize(isect.position - isects[depth - 1].position);
		float3 wi;
		float3 weight;
		isect.material->sample(isectShadingCS.local(woWorld), rand.next01f2(), &wi, &weight);
		float3 wiWorld = isectShadingCS.world(wi);			
		T[depth + 1] = T[depth] * weight;

		ray.origin = isect.position;		
		ray.dir = wiWorld;
		

		/*
		ray.origin = isect.position;		
		ShadingCS isectShadingCS(isect.normal);
		ray.dir = isectShadingCS.world(sampleHemisphere(rand));
		*/
	}
	return numIsects;
}


bool isBlack( const float3& color ) 
{
	return color.x == 0 && color.y == 0 && color.z == 0;
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
void expect(int a, int b, const char* msg)
{
	if(a != b) cout << msg << endl;
}
void bdptRun(const RTScene& scene, int bounces, Rand& rand, Sample* sample)
{
	
	PathVertex lpVerts[16];
	PathVertex epVerts[16];

	//we disregard direct light->eye connection
	//we include direct eye->light connection
	int maxEyeVertices = bounces + 1; //explicit vertices (the one at the eye is implicit)
	int maxLightVertices = bounces;
	int numLPV = createLightPath2(scene, rand, maxLightVertices, lpVerts);
	//starts at 2 vertices
	int numEPV = createEyePath(scene, rand, maxEyeVertices, sample->ray, epVerts);
	//maps path length to deltas
	//lowest = 1 vertices (1 eye, 0 light)
	/*
	int deltasForPathLength[16];
	for(int i = 0; i < 16; i++) deltasForPathLength[i] = 0;	
	for(int epvIdx = 0; epvIdx < numEPV; epvIdx++)
	{
		auto & ev = epVerts[epvIdx];		
		if(!isBlack(ev.material->emission)) continue; //we're the direct eye-light connection and hit a light
		if(epvIdx == (maxEyeVertices - 1)) continue; //we're the direct eye-light connection but didn't hit a light

		//this path has a shadow ray...
		for(int lpvIdx = 0; lpvIdx < numLPV; lpvIdx++)
		{			
			auto & lv = lpVerts[lpvIdx];
			int evCount = epvIdx + 2;
			int lvCount = lpvIdx + 1;
			//total vertices count
			int vCount = evCount + lvCount;
			int pathLength = vCount - 1;
			if(ev.isDelta() || lv.isDelta()) deltasForPathLength[pathLength]++;
			
		}
		
	}
	*/
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
	
	/*
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
		for(int lightPathIsectIdx = 0; lightPathIsectIdx < numLPV; lightPathIsectIdx++)			
		{
			//there's a shadow ray here! (so +1)
			int pathLength = (lightPathIsectIdx) + (depth + 1) + 1;
			//todo: make the following depend on Russian Roulette data
			//we lose a variability b/c our first eye path is not removable

			float bidirectionalWeight = 1.f / (pathLength);
			
			
			if(pathLength > (bounces + 1)) continue;

			const auto & lv = lpVerts[lightPathIsectIdx];
			if(visibleAndFacing(
				lv.position,
				lv.normal, 
				sceneIsect.position, 
				sceneIsect.normal, scene))
			{
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
					float3 woLV = lvShadingCS.local(lv.woWorld);
					lvBrdfEval = lv.material->eval(wiLV, woLV);
				}
						
				//geometry term
				float g = dot(ev2lv, ev.normal) * dot(-ev2lv, lv.normal) / glm::distance2(lv.position, ev.position);
				float3 lpT = lvBrdfEval * lv.throughput;
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
	*/
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