#include "stdafx.h"
#include "scene.h"

#include "uniformgrid.h"
embree::Vec3f toEmbree(float3 v)
{
	return embree::Vec3f(v.x, v.y, v.z);
}
Intersection AccelScene::intersect(const IntersectionQuery& query) const
{
	embree::Ray ray(toEmbree(query.ray.origin), toEmbree(query.ray.dir), query.minT, query.maxT);
	embree::Hit sceneHit;
	intersector->intersect(ray, sceneHit);
	
	Intersection lightIsect;
	bool hitLight = false;
	if(!query.ignoreLights)
	{
		hitLight = lights[0].intersect(query, &lightIsect);
		//make sure the light hit is valid
		hitLight = hitLight && query.isValid(lightIsect);
	}
	if(sceneHit && ((!hitLight) || (lightIsect.t > sceneHit.t)))
	{
		int triIdx = sceneHit.id0;
		const Triangle& tri = data->triangles[triIdx];
		Intersection isect;
		isect.t = sceneHit.t;
		isect.hit = true;
		isect.material = tri.material;
		isect.normal = tri.normal;
		isect.position = query.ray.at(sceneHit.t);
		return isect;		
	}
	else if(hitLight)
	{
		return lightIsect;
	}
	else
	{
		Intersection isect;
		isect.hit = false;
		return isect;
	}
}
unique_ptr<AccelScene> makeAccelScene(const StaticScene* scene)
{
	embree::vector_t<embree::BuildTriangle> buildTris;
	embree::TaskScheduler::init();
	for(int triIdx = 0; triIdx < scene->triangles.size(); triIdx++)
	{
		auto & sourceTri = scene->triangles.at(triIdx);
		embree::BuildTriangle buildTri(
			toEmbree(sourceTri.vertices[0].position), 
			toEmbree(sourceTri.vertices[1].position), 
			toEmbree(sourceTri.vertices[2].position), 
			triIdx,
			0);
		buildTris.push_back(buildTri);
	}
	unique_ptr<AccelScene> accelScene(new AccelScene());
	accelScene->data = scene;
	accelScene->intersector = embree::rtcCreateAccel("bvh2", 
		(const embree::BuildTriangle*)buildTris.begin(), buildTris.size());
	accelScene->lights[0] = createDefaultLight();
	return accelScene;
}