#pragma once
#include "lwmath.h"
#include <algorithm>
using namespace std;
#include "debugutils.h"
/*
struct ___Brdf
{
	//generate random directions + weight (brdf * cos) / p
	void sample(const float3& wo, const vec2& rand, float3* wi, float3* weight) const;

	//get the prob. for a particular direction
	float pdf(const float3& wi, const float3& wo) const;

	//evaluate the pdf
	float3 eval(const float3& wi, const float3& wo) const;

	//everything is z-up
};
*/
struct Fresnel
{	
	float3 f0;
	float3 eval(float idoth) const
	{
		//cosi must be > 0
		LWASSERT_VALIDFLOAT(idoth);
		if(idoth > 0) return f0 + (float3(1)-f0) * float3(pow(1-idoth, 5));
		lwassert(false);
		return float3(0);
	}
};
struct PerfectSpecular
{
	//generate random directions + weight (brdf * cos) / p
	void sample(const float3& wo, const float2& rand, float3* wi, float3* weight) const
	{
		*wi = glm::reflect(-wo, float3(0, 0, 1));
		lwassert_greater(wi->z, 0);
		// TODO: not implemented
		lwassert(0); 
	}

	//get the prob. for a particular direction
	float pdf(const float3& wi, const float3& wo) const
	{
		return 0.f;
	}

	//evaluate the pdf
	float3 eval(const float3& wi, const float3& wo) const
	{
		return float3(0);
	}
	//everything is z-up
};
struct BlinnPhongBrdf
{
	BlinnPhongBrdf() : spec_power(10) { }
	void sample(const float3& wo, const float2& rand, float3* wi, float3* weight) const
	{
		
			lwassert_greater(spec_power, 0);

		float costheta = powf(rand.x, 1.f / (spec_power+1));
			LWASSERT_VALIDVEC(wo);
			LWASSERT_VALIDFLOAT(costheta);
			
			//float k = std::acos(1.f);//0.0f, );
		float sintheta = sqrtf(1.f-costheta*costheta);
			LWASSERT_VALIDFLOAT(sintheta);
		float phi = rand.y * 2 * PI;
			LWASSERT_VALIDFLOAT(phi);
		float3 h = zup::sph2cart(sintheta, costheta, phi);
			lwassert_validvec(h);
		bool flipped = false;
		if(!zup::same_hemi(wo, h)) 
		{
			h = -h; flipped = true; 
		}

		*wi = glm::reflect(-wo, h);
			lwassert_validvec(*wi);
		
			lwassert_greater(wo.z, 0);
		if(zup::cos_theta(*wi) <= 0)
		{
			*weight = float3(0); return;
		}
			lwassert_greater(wi->z, 0);

		float prob = pdf(*wi, wo);
			LWASSERT_VALIDFLOAT(prob);
		
		*weight = eval(*wi, wo) * zup::cos_theta(*wi) / prob;
			lwassert_validvec(*weight);
			lwassert_greater(prob, 0);
			lwassert_allgreater(*weight, float3(0));//weight->x > 0 && weight->y > 0 && weight->z > 0);
			
	}
	float pdf(const float3& wi, const float3& wo) const
	{
		float3 h = glm::normalize(wi + wo);
		lwassert_validvec(h);
		float cos_theta_hi = glm::dot(h, wi);
		if(cos_theta_hi <=0) return 0;
		return (spec_power+1)*pow(zup::cos_theta(h), spec_power)
			/ (2 * PI * 4 * cos_theta_hi);
	}
	float3 eval(const float3& wi, const float3& wo) const
	{		
			lwassert_greater(spec_power, 0);
			lwassert_validvec(wo);
			lwassert_validvec(wi);
		float3 h = glm::normalize(wi + wo);
			lwassert_validvec(h);
		//float3 f = f0 + (float3(1) - f0) * float3(glm::pow((float)(1 - glm::dot(wi, h)), (float)5));
		//	lwassert_validvec(f);
		float3 d = float3((spec_power + 2) / (2 * PI) * pow(zup::cos_theta(h), spec_power));
			lwassert_validvec(d);
		return d;
	}
	float spec_power;
};
/*
struct Btdf
{
	Fresnel fresnel;
	Btdf() : enabled(false) { }
	bool enabled;
	void sample(const float3& wo, const float2& rand, float3* wi, float3* weight) const
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
		*wi = float3(sino_over_sini * -wo.x, sino_over_sini * -wo.y, coso);
		if(sino2 >= 1)
		{
			*weight = float3(0);
		}
		else
		{
			*weight = float3((eo * eo) / (ei * ei)) * 
				float3(zup::abs_cos_theta(*wi));
		}
	}
private:	
};
*/

struct Dielectric
{
	BlinnPhongBrdf reflection;
	//Btdf refraction;
	/*
	const Fresnel* fresnel()
	{
		return &refraction.fresnel;
	}*/
};
struct LambertBrdf
{	
	LambertBrdf() : albedo(0) { }
	void sample(const float2& rand, float3* wi, float3* weight) const
	{
		float a = sqrt(1 - rand.y);

		*wi = float3(cos(2 * PI * rand.x) * a, sin(2 * PI * rand.x) * a, sqrt(rand.y));

		//1/pi * (1/(costheta/pi)) * costheta = 1;
		LWASSERT_ALLGE(albedo, float3(0));
		*weight = albedo;
	}
	float pdf(const float3& wi) const
	{
		return INV_PI * zup::cos_theta(wi);
	}
	float3 eval() const
	{
		return albedo * INV_PI_V3;
	}
	float3 albedo;
};
struct FresnelBlendBrdf
{
	FresnelBlendBrdf() { }
	// weight (brdf * cos) / p
	void sample(const float3& wo, float2& rand, float3* wi, float3* weight) const
	{
		float3 dummy;
		bool phong;
		if(rand.x > 0.5f)
		{
			rand.x = (rand.x - 0.5f) * 2;
			//phong
			phongBrdf.sample(wo, rand, wi, &dummy);
			phong = true;
		}
		else
		{
			//lambert
			rand.x = rand.x * 2;
			lambertBrdf.sample(rand, wi, &dummy);
			phong = false;
		}	
		if(glm::all(glm::lessThanEqual(dummy, float3(0))))
		{
			*weight = float3(0);
		}
		else
		{
			*weight = eval(*wi, wo) * zup::cos_theta(*wi) / pdf(*wi, wo);
		}
	}
	float pdf(const float3& wi, const float3& wo) const
	{
		float result = 0.5f * (phongBrdf.pdf(wi, wo) + lambertBrdf.pdf(wi));
		if(phongBrdf.pdf(wi, wo)  <= 0)
		{
			cout << "";
		}
		return 0.5f * (phongBrdf.pdf(wi, wo) + lambertBrdf.pdf(wi));
	}
	float3 eval(const float3& wi, const float3& wo) const
	{				
		float3 h = normalize(wi + wo);
		float idoth = glm::dot(wi, h);
		float3 f = fresnel.eval(idoth);

		float3 diffuseEval = lambertBrdf.eval();		
		float3 phongEval = phongBrdf.eval(wi, wo);

		return  
			f * phongEval / (float3(4) * zup::cos_theta(wi) * zup::cos_theta(wo))
			+ (float3(1)-fresnel.f0) * diffuseEval;
	}	
	BlinnPhongBrdf phongBrdf;
	LambertBrdf lambertBrdf;
	Fresnel fresnel;
};