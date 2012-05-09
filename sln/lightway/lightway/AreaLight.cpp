#include "stdafx.h"
#include "AreaLight.h"
#include "shapes.h"
#include "bxdf.h"

float3 RectangularAreaLight::samplePoint(Rand& rand) const
{
    float r0 = rand.norm_unif_rand(rand.gen);
    float r1 = rand.norm_unif_rand(rand.gen);
        
    float3 pt0 = (corners[0] - corners[3]) * r0 + corners[3];
    float3 pt1 = (corners[1] - corners[2]) * r0 + corners[2];
    float3 pt = r1 * (pt1 - pt0) + pt0;
    return pt;    
}
void RectangularAreaLight::sample(Rand& rand, const float3& pos, float3* lightPos, float3* wiWorld, float* pdf, float* t) const
{
	*lightPos = samplePoint(rand);
	*wiWorld = normalize(*lightPos - pos);
	*t = glm::length(*lightPos - pos);
	*pdf = this->pdf(*wiWorld, *t);
}
float RectangularAreaLight::pdf(const float3& wiWorld, float d) const
{
	return (d * d) / (dot(normal, -wiWorld) * area());
}
float RectangularAreaLight::pdf(const IntersectionQuery& query) const
{
	//find if the ray intersects
	Intersection isectSelf;
	bool hit = intersect(query, &isectSelf);
	//if it intersects, find the pt. of intersection and the distance
	if(!hit) return 0;
	//call pdf()
	return pdf(query.ray.dir, isectSelf.t);
}
float RectangularAreaLight::area() const
{
    return length(corners[0] - corners[3]) * length(corners[1] - corners[2]);
}
const float IN_RECT_ANGLE_EPSILON = 0.00001;
bool RectangularAreaLight::intersect(const IntersectionQuery& query, Intersection* intersection) const
{
	//normally, ray and normal's directions have to be opposite
	if(dot(query.ray.dir, normal) > -query.minCosTheta) return false;
	float rdotn = dot(query.ray.dir, normal);

	float d = dot(corners[0] - query.ray.origin, normal) / rdotn;

	if(d < query.minT || d > query.maxT) return false;

	float3 pt = query.ray.at(d);
	if((dot(normalize(pt - corners[0]), normalize(corners[1] - corners[0]))) < -IN_RECT_ANGLE_EPSILON) return false;
	if((dot(normalize(pt - corners[1]), normalize(corners[2] - corners[1]))) < -IN_RECT_ANGLE_EPSILON) return false;
	if((dot(normalize(pt - corners[2]), normalize(corners[3] - corners[2]))) < -IN_RECT_ANGLE_EPSILON) return false;
	if((dot(normalize(pt - corners[3]), normalize(corners[0] - corners[3]))) < -IN_RECT_ANGLE_EPSILON) return false;

	intersection->hit = true;
	intersection->material = &material;
	intersection->normal = (normal);
	intersection->position = pt;
	intersection->t = d;
	intersection->lightId = id;
	return true;
} 

RectangularAreaLight createDefaultLight()
{	
	RectangularAreaLight light;
	float3 base(0, 0, 0);
	float y = .35;
	/*
	float3 light_verts[] = {
		base + float3(-.2, y, -.2),
		base + float3(-.2, y, .2),
		base + float3(.2, y, .2),
		base + float3(.2, y, -.2)
	};
	*/
	
	float3 light_verts[] = {
		base + float3(-.125, y, -.125),
		base + float3(-.125, y, .125),
		base + float3(.125, y, .125),
		base + float3(.125, y, -.125)
	};
	
	
    light.corners[0] = light_verts[0];//float3(-1, 39.5, -1);
    light.corners[1] = light_verts[1];//float3(1, 39.5, -1);
    light.corners[2] = light_verts[2];//float3(1, 39.5, 1);
    light.corners[3] = light_verts[3];//float3(-1, 39.5, 1);
    light.normal = float3(0, -1, 0);
	
	light.material.emission = float3(6);
	//light.material.emission = float3(2);
	return light;
}