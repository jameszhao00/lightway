#pragma once
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "math.h"
#include "scene.h"
using namespace glm;
struct Intersection;
struct Material;
struct Disc
{
    Disc()// : Disc(vec3(DEBUGVAL), vec3(DEBUGVAL), DEBUGVAL, (Material*)DEBUGVAL)
	{
		Disc(vec3(DEBUGVAL), vec3(DEBUGVAL), DEBUGVAL, (Material*)DEBUGVAL);
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
		Sphere(vec3(DEBUGVAL), DEBUGVAL, (Material*)DEBUGVAL);
	}
	Sphere(const vec3& c, const float& r, const Material* m) 
	    : center(c), radius(r), material(m) { }
	vec3 center;
	float radius;
    const Material* material;
};
struct Ray
{
    Ray() //: Ray(vec3(DEBUGVAL), vec3(DEBUGVAL)) { }
	{
		Ray(vec3(DEBUGVAL), vec3(DEBUGVAL));
	}
    Ray(const vec3& o, const vec3& d) : origin(o), dir(d) { }
	vec3 origin;
	vec3 dir;
    void intersect_with_discs(const Disc* discs, const int num_discs, Intersection* intersections) const;
    void intersect_with_spheres(const Sphere* spheres, const int num_spheres, Intersection* intersections) const;
	void intersect_with_triangles(const Triangle* triangles, const int num_tris, Intersection* intersections, bool flip_ray) const;        
};