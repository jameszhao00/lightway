#pragma once
#include "math.h"
#include "scene.h"
using namespace glm;
struct Intersection;
struct Material;
struct Disc
{
    Disc()// : Disc(vec3(DEBUGVAL), vec3(DEBUGVAL), DEBUGVAL, (Material*)DEBUGVAL)
	{
		this->Disc::Disc(vec3(DEBUGVAL), vec3(DEBUGVAL), DEBUGVAL, (Material*)DEBUGVAL);
	}
	Disc(const vec3& c, const vec3& n, float r, const Material* m) 
	    : center(c), normal(n), radius(r), material(m) { }
	vec3 center;
	vec3 normal;
	float radius;
    const Material* material;
};
struct Sphere
{
    Sphere()// : Sphere(vec3(DEBUGVAL), DEBUGVAL, (Material*)DEBUGVAL){ }
	{
		this->Sphere::Sphere(vec3(DEBUGVAL), DEBUGVAL, (Material*)DEBUGVAL);
	}
	Sphere(const vec3& c, const float& r, const Material* m) 
	    : center(c), radius(r), material(m) { }
	vec3 center;
	float radius;
    const Material* material;
};
struct AABB
{
	AABB() { }
	AABB(const vec3& p_min_pt, const vec3& p_max_pt) 
		: max_pt(p_max_pt), min_pt(p_min_pt), extent(p_max_pt-p_min_pt) { }

	//which side is the touching point on?
	ivec3 side(const vec3& touching_pt, float epsilon = 0.00001) const
	{
		if(equal(touching_pt.x, min_pt.x, epsilon)) return ivec3(-1, 0, 0);
		if(equal(touching_pt.x, max_pt.x, epsilon)) return ivec3(1, 0, 0);
		if(equal(touching_pt.y, min_pt.y, epsilon)) return ivec3(0, -1, 0);
		if(equal(touching_pt.y, max_pt.y, epsilon)) return ivec3(0, 1, 0);
		if(equal(touching_pt.z, min_pt.z, epsilon)) return ivec3(0, 0, -1);
		if(equal(touching_pt.z, max_pt.z, epsilon)) return ivec3(0, 0, 1);
		return ivec3(INF);
	}
	vec3 extent;
	vec3 max_pt;
	vec3 min_pt;
};
struct Ray
{
    Ray() //: Ray(vec3(DEBUGVAL), vec3(DEBUGVAL)) { }
	{
		this->Ray::Ray(vec3(DEBUGVAL), vec3(DEBUGVAL));
	}
    Ray(const vec3& o, const vec3& d) : origin(o), dir(d) { }
	vec3 origin;
	vec3 dir;
	vec3 at(float t) const;
    void intersect_with_discs(const Disc* discs, const int num_discs, Intersection* intersections) const;
    void intersect_with_spheres(const Sphere* spheres, const int num_spheres, Intersection* intersections) const;
	void intersect_with_triangles(const Triangle* triangles, const int num_tris, Intersection* intersections, bool flip_ray) const;        
	
	bool intersect_with_aabb(const AABB& aabb, float* t, float* t_max_out) const;
};
