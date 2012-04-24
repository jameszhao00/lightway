#pragma once
#include "math.h"
#include <algorithm>
using namespace std;
/*
struct ___Brdf
{
	//generate random directions + weight (brdf * cos) / p
	void sample(const vec3& wo, const vec2& rand, vec3* wi, vec3* weight) const;

	//get the prob. for a particular direction
	float pdf(const vec3& wi, const vec3& wo) const;

	//evaluate the pdf
	vec3 eval(const vec3& wi, const vec3& wo) const;

	//everything is z-up
};
*/
namespace zup // z up
{
	inline float cos_theta(const vec3& v) { return v.z; }
	inline float abs_cos_theta(const vec3& v) { return glm::abs(cos_theta(v)); }

	inline float sin_theta2(const vec3& v) { return 1.0f - v.z * v.z; }
	inline float sin_theta(const vec3& v) 
	{ 
		float temp = sin_theta2(v);
		if (temp <= 0.0f) return 0.0f;
		else return std::sqrt(temp);
	}	
	inline vec3 sph2cart(float sintheta, float costheta, float phi)
	{
		return vec3(sintheta * cos(phi), sintheta * sin(phi), costheta);
	}
};
struct Fresnel
{	
	float eta_inside, eta_outside;
	float f0;
	void cache_f0() 
	{
		f0 = pow((eta_outside - eta_inside) / (eta_outside + eta_inside), 2); 
	}
	float eval(float half_odoth) const
	{
		//cosi must be > 0
		if(half_odoth > 0) return f0 + (1-f0) * pow(1-half_odoth, 5);
		return 1000000;
	}
};
struct BlinnPhongBrdf
{
	void sample(const vec3& wo, const vec2& rand, vec3* wi, vec3* weight) const
	{
		float costheta = powf(rand.x, 1.f / (spec_power+1));
		float sintheta = sqrtf(glm::max(0.0f, 1-costheta*costheta));
		float phi = rand.y * 2 * PI;
		vec3 h = zup::sph2cart(sintheta, costheta, phi);
		if(dot(h, wo) < 0) h = -h;

		*wi = reflect(-wo, h);
		*weight = eval(*wi, wo) * zup::cos_theta(*wi) / pdf(*wi, wo);
	}
	float pdf(const vec3& wi, const vec3& wo) const
	{
		vec3 h = normalize(wi + wo);
		float cos_theta_hi = dot(h, wi);
		return (spec_power+1)*pow(zup::cos_theta(h), spec_power)
			/ (2 * PI * 4 * cos_theta_hi);
	}
	vec3 eval(const vec3& wi, const vec3& wo) const
	{		
		vec3 h = normalize(wi + wo);
				
		vec3 f = f0 + (vec3(1) - f0) * vec3(glm::pow((float)(1 - dot(wi, h)), (float)5));
		vec3 d = vec3((spec_power + 2) / (2 * PI) * pow(zup::cos_theta(h), spec_power));
		return (f * d) / vec3(4);
	}
	vec3 f0;
	float spec_power;
};
struct Btdf
{
	Fresnel fresnel;
	Btdf() : enabled(false) { }
	bool enabled;
	void sample(const vec3& wo, const vec2& rand, vec3* wi, vec3* weight) const
	{
		bool entering = zup::cos_theta(wo) > 0;
		float ei = fresnel.eta_outside, eo = fresnel.eta_inside;
		if(!entering) swap(ei, eo);
		
		float sini2 = zup::sin_theta2(wo);
		float eta = ei / eo;
		float sino2 = eta * eta * sini2;
		float coso = sqrtf(glm::max(0.0f, 1.0f - sino2));
		if(entering) coso = -coso;
		float sino_over_sini = eta;
		*wi = vec3(sino_over_sini * -wo.x, sino_over_sini * -wo.y, coso);
		if(sino2 >= 1)
		{
			*weight = vec3(0);
		}
		else
		{
			*weight = vec3((eo * eo) / (ei * ei)) * 
				vec3(zup::abs_cos_theta(*wi));
		}
	}
private:	
};

struct Dielectric
{
	BlinnPhongBrdf reflection;
	Btdf refraction;
	const Fresnel* fresnel()
	{
		return &refraction.fresnel;
	}
};
struct LambertBrdf
{	
	void sample(const vec2& rand, vec3* wi, vec3* weight) const
	{
		float a = sqrt(1 - rand.y);

		*wi = vec3(cos(2 * PI * rand.x) * a, sin(2 * PI * rand.x) * a, sqrt(rand.y));

		//1/pi * (1/(costheta/pi)) * costheta = 1;
		*weight = albedo;
	}
	float pdf(const vec3& wi) const
	{
		return INV_PI * zup::cos_theta(wi);
	}
	vec3 eval() const
	{
		return albedo * INV_PI_V3;
	}
	vec3 albedo;
};