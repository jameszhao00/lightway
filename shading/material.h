#pragma once
#include "../math.h"
#include "../bxdf.h"
struct Material
{
	LambertBrdf lambert;
	BlinnPhongBrdf phong;
	Btdf refraction;
	
	/*
	vec3 radiance(vec3 i, vec3 o, vec3 n, vec3 radiance, bool visibility, bool use_fresnel = true) const
	{
        vec3 scale(1);
		return vec3(visibility) * scale * brdf(i, o, n, use_fresnel) * radiance * vec3(saturate(dot(n, -i)));
	}
	vec3 spec_brdf(vec3 i, vec3 o, vec3 n, bool use_fresnel = true) const
	{    
		vec3 h = (-i + o);
		h = normalize(h);
		auto specular = phong.f0;
		auto roughness = phong.spec_power;
		vec3 f = specular + (vec3(1) - specular) * vec3(glm::pow((float)(1 - dot(-i, h)), (float)5));
		vec3 d = vec3((roughness + 2) / (2 * PI) * pow(dot(n, h), roughness));
		if(!use_fresnel) f = vec3(1);
		return (f * d) / vec3(4);
	}
	vec3 diff_brdf() const
	{
		auto specular = phong.f0;
		auto roughness = phong.spec_power;
		auto albedo = lambert.albedo;
        return (vec3(1) - specular) * (albedo / vec3(PI));
	}
	vec3 brdf(vec3 i, vec3 o, vec3 n, bool use_fresnel = true) const
	{
        return spec_brdf(i, o, n, use_fresnel) + diff_brdf();
	}
	*/
	
};