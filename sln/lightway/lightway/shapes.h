#pragma once
#include "lwmath.h"
#include "scene.h"
struct Intersection;
struct Material;
struct Triangle;
struct Disc
{
    Disc()// : Disc(float3(DEBUGVAL), float3(DEBUGVAL), DEBUGVAL, (Material*)DEBUGVAL)
	{
		this->Disc::Disc(float3(DEBUGVAL), float3(DEBUGVAL), DEBUGVAL, (Material*)DEBUGVAL);
	}
	Disc(const float3& c, const float3& n, float r, const Material* m) 
	    : center(c), normal(n), radius(r), material(m) { }
	float3 center;
	float3 normal;
	float radius;
    const Material* material;
};
struct Sphere
{
    Sphere()// : Sphere(float3(DEBUGVAL), DEBUGVAL, (Material*)DEBUGVAL){ }
	{
		this->Sphere::Sphere(float3(DEBUGVAL), DEBUGVAL, (Material*)DEBUGVAL);
	}
	Sphere(const float3& c, const float& r, const Material* m) 
	    : center(c), radius(r), material(m) { }
	float3 center;
	float radius;
    const Material* material;
};
struct AABB
{
	AABB() { }
	AABB(const float3& p_min_pt, const float3& p_max_pt) 
		: max_pt(p_max_pt), min_pt(p_min_pt), extent(p_max_pt-p_min_pt) { }

	//which side is the touching point on?
	int3 side(const float3& touching_pt, float epsilon = 0.00001) const
	{
		if(equal(touching_pt.x, min_pt.x, epsilon)) return int3(-1, 0, 0);
		if(equal(touching_pt.x, max_pt.x, epsilon)) return int3(1, 0, 0);
		if(equal(touching_pt.y, min_pt.y, epsilon)) return int3(0, -1, 0);
		if(equal(touching_pt.y, max_pt.y, epsilon)) return int3(0, 1, 0);
		if(equal(touching_pt.z, min_pt.z, epsilon)) return int3(0, 0, -1);
		if(equal(touching_pt.z, max_pt.z, epsilon)) return int3(0, 0, 1);
		return int3(INF);
	}
	float3 extent;
	float3 max_pt;
	float3 min_pt;
};
struct Ray
{
    Ray() //: Ray(float3(DEBUGVAL), float3(DEBUGVAL)) { }
	{
		this->Ray::Ray(float3(DEBUGVAL), float3(DEBUGVAL));
	}
    Ray(const float3& o, const float3& d) : origin(o), dir(d) { }
	float3 origin;
	float3 dir;
	float3 at(float t) const;
    void intersect_with_discs(const Disc* discs, const int num_discs, Intersection* intersections) const;
    void intersect_with_spheres(const Sphere* spheres, const int num_spheres, Intersection* intersections) const;
	void intersect_with_triangles(const Triangle* triangles, const int num_tris, Intersection* intersections, bool flip_ray) const;        
	
	bool intersect_with_aabb(const AABB& aabb, float* t, float* t_max_out) const;
};

struct IntersectionQuery
{
	IntersectionQuery(const Ray& pRay, bool pFlipRay, bool ignoreLights, float pMaxT = INF, float pMinT = 0.00001f, float pMinCosTheta = 0.00001f) :	
		ray(pRay), flipRay(pFlipRay), maxT(pMaxT), minT(pMinT), minCosTheta(pMinCosTheta), ignoreLights(ignoreLights)
	{
	}
		
	Ray ray;
	float maxT;
	float minT;
	float minCosTheta;
	bool flipRay;
	bool ignoreLights;

	bool isValid(const Intersection& intersection) const
	{
		bool result = intersection.t < maxT && 
			intersection.t > minT &&
			fabs(dot(ray.dir, intersection.normal)) > minCosTheta;
		return result;
	}
};