#pragma once
#include "lwmath.h"
#include "shapes.h"

struct Material;
struct Intersection;
struct IntersectionQuery;
struct RectangularAreaLight
{
    RectangularAreaLight() : normal(0), material(nullptr) { }

	void sample(Rand& rand, const float3& pos, float3* lightPos, float3* wiWorld, float* pdf, float* t) const;
	float pdf(const float3& wiWorld, float d) const;
	float pdf(const IntersectionQuery& query) const; 
    float area() const;
	bool intersect(const IntersectionQuery& query, Intersection* intersection) const;

    float3 corners[4];
    float3 normal;
	const Material *material;
private:	
    float3 samplePoint(Rand& rand) const;
};