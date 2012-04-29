#pragma once
#include "math.h"
#include <algorithm>
using namespace std;
using namespace glm;
#include "debugutils.h"
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
		lwassert_validvec(v);
		float temp = sin_theta2(v);
		lwassert_validfloat(temp);
		if (temp <= 0.0f) return 0.0f;
		else return std::sqrt(temp);
	}	
	inline vec3 sph2cart(float sintheta, float costheta, float phi)
	{
		lwassert_validfloat(sintheta);
		lwassert_validfloat(costheta);
		lwassert_validfloat(phi);
		return vec3(sintheta * cos(phi), sintheta * sin(phi), costheta);
	}
	inline bool same_hemi(const vec3& a, const vec3& b)
	{
		lwassert_validvec(a);
		lwassert_validvec(b);
		return a.z * b.z > 0;
	}
};
struct Fresnel
{	
	float eta_inside, eta_outside;
	float f0;
	void cache_f0() 
	{
		f0 = pow((eta_outside - eta_inside) / (eta_outside + eta_inside), 2); 		
		lwassert_validfloat(f0);
	}
	float eval(float half_odoth) const
	{
		//cosi must be > 0
		lwassert_validfloat(half_odoth);
		if(half_odoth > 0) return f0 + (1-f0) * pow(1-half_odoth, 5);		
		lwassert(false);
		return 1000000;
	}
};
struct BlinnPhongBrdf
{
	BlinnPhongBrdf() : f0(0.02), spec_power(10) { }
	void sample(const vec3& wo, Rand& rand, vec3* wi, vec3* weight) const
	{
		lwassert_greater(spec_power, 0);

		float r1 = rand.next01(); float r2 = rand.next01();
		float costheta = powf(r1, 1.f / (spec_power+1));
		lwassert_validvec(wo);
		lwassert_validfloat(costheta);
		float sintheta = sqrtf(glm::max(0.0f, 1-costheta*costheta));
		lwassert_validfloat(sintheta);
		float phi = r2 * 2 * PI;
		lwassert_validfloat(phi);
		vec3 h = zup::sph2cart(sintheta, costheta, phi);
		lwassert_validvec(h);
		bool flipped = false;
		if(!zup::same_hemi(wo, h)) {h = -h; flipped = true; }
		*wi = reflect(-wo, h);
		lwassert_validvec(*wi);
		
		lwassert_greater(wo.z, 0);
		if(zup::cos_theta(*wi) <= 0)
		{
			*weight = vec3(0); return;
		}
		lwassert_greater(wi->z, 0);

		float prob = pdf(*wi, wo);
		lwassert_validfloat(prob);
		
		*weight = eval(*wi, wo) * zup::cos_theta(*wi) / prob;
		lwassert_validvec(*weight);
		lwassert_greater(prob, 0);
		lwassert_allgreater(*weight, vec3(0));//weight->x > 0 && weight->y > 0 && weight->z > 0);
	}
	float pdf(const vec3& wi, const vec3& wo) const
	{
		vec3 h = normalize(wi + wo);
		lwassert_validvec(h);
		float cos_theta_hi = dot(h, wi);
		lwassert_greater(cos_theta_hi, 0);
		return (spec_power+1)*pow(zup::cos_theta(h), spec_power)
			/ (2 * PI * 4 * cos_theta_hi);
	}
	vec3 eval(const vec3& wi, const vec3& wo) const
	{		
		lwassert_greater(spec_power, 0);
		lwassert_validvec(wo);
		lwassert_validvec(wi);
		vec3 h = normalize(wi + wo);
		lwassert_validvec(h);
				
		vec3 f = f0 + (vec3(1) - f0) * vec3(glm::pow((float)(1 - dot(wi, h)), (float)5));
		lwassert_validvec(f);
		vec3 d = vec3((spec_power + 2) / (2 * PI) * pow(zup::cos_theta(h), spec_power));
		lwassert_validvec(d);
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
	LambertBrdf() : albedo(0) { }
	void sample(const vec2& rand, vec3* wi, vec3* weight) const
	{
		float a = sqrt(1 - rand.y);

		*wi = vec3(cos(2 * PI * rand.x) * a, sin(2 * PI * rand.x) * a, sqrt(rand.y));

		//1/pi * (1/(costheta/pi)) * costheta = 1;
		LWASSERT_ALLGE(albedo, vec3(0));
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