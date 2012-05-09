#include "stdafx.h"
#include "bxdf.h"


void Material::sample(const float3& wo, const float2& rand, float3* wi, float3* weight) const
{
	if(type == Diffuse)
	{
		return diffuse.sample(rand, wi, weight);
	}
	else if(type == PerfectReflection)
	{
		return specular.sample(wo, wi, weight);
	}
}
float Material::pdf(const float3& wi, const float3& wo) const
{
	if(type == Diffuse)
	{
		return diffuse.pdf(wi);
	}
	else if(type == PerfectReflection)
	{
		return specular.pdf();
	}
	lwassert(false);
	return -1;
}
float3 Material::eval(const float3& wi, const float3& wo) const
{
	if(type == Diffuse)
	{
		return diffuse.eval();
	}
	else if(type == PerfectReflection)
	{
		return specular.eval();
	}
	lwassert(false);
	return float3(-1);
}
bool Material::isDelta() const
{
	if(type == Diffuse)
	{
		return diffuse.isDelta();
	}
	else if(type == PerfectReflection)
	{
		return specular.isDelta();
	}
	lwassert(false);
	return false;
}