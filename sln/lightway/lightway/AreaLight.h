#pragma once
#include "lwmath.h"
#include "bxdf.h"

const int INVALID_LIGHT_IDX = -1;
inline bool validLightIdx(int idx) { return idx > -1; }
struct Material;
struct Intersection;
struct IntersectionQuery;
struct RectangularAreaLight
{
    RectangularAreaLight(int pIdx = 0) : normal(0), idx(pIdx) 
	{
		lwassert(pIdx > -1);
	}
	void sample(Rand& rand, const float3& pos, float3* lightPos, float3* wiWorld, float* pdf, float* t) const;
	float pdf(const float3& wiWorld, float d) const;
	float pdf(const IntersectionQuery& query) const; 
    float area() const;
	bool intersect(const IntersectionQuery& query, Intersection* intersection) const;
    float3 samplePoint(Rand& rand) const;

    float3 corners[4];
    float3 normal;
	Material material;
	int idx;
private:	
};

RectangularAreaLight createDefaultLight();