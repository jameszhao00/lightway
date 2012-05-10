#include "stdafx.h"
#include "bxdf.h"


void Material::sample(const float3& wo, const float2& rand, float3* wi, float3* weight) const
{
	if(type == Diffuse)
	{
		return diffuse.sample(rand, wi, weight);
	}
	else if(type == SpecularReflection)
	{
		return specularReflection.sample(wo, wi, weight);
	}
	else if(type == SpecularTransmission)
	{
		return specularTransmission.sample(wo, wi, weight);
	}
}
float Material::pdf(const float3& wi, const float3& wo) const
{
	if(type == Diffuse)
	{
		return diffuse.pdf(wi);
	}
	else if(type == SpecularReflection)
	{
		return specularReflection.pdf();
	}
	else if(type == SpecularTransmission)
	{
		return specularTransmission.pdf();
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
	else if(type == SpecularReflection)
	{
		return specularReflection.eval();
	}
	else if(type == SpecularTransmission)
	{
		return specularTransmission.eval();
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
	else if(type == SpecularReflection)
	{
		return specularReflection.isDelta();
	}
	else if(type == SpecularTransmission)
	{
		return specularTransmission.isDelta();
	}
	lwassert(false);
	return false;
}