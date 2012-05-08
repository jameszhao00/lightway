#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <random>
#include <limits>


typedef glm::mat4 float4x4;
typedef glm::mat3 float3x3;
typedef glm::ivec2 int2;
typedef glm::ivec3 int3;
typedef glm::vec3 float3;
typedef glm::ivec4 int4;
typedef glm::vec4 float4;
typedef glm::vec2 float2;
#define PI 3.1415f
#define INV_PI 1/PI
#define INV_PI_V3 float3(INV_PI)
#define DEBUGVAL -5000000

template<typename T>
float dot(const T& a, const T& b) { return glm::dot(a, b); }
template<typename T>
T cross(const T& a, const T& b) { return glm::cross(a, b); }
template<typename T>
T normalize(const T& v) { return glm::normalize(v); }
template<typename T>
T saturate(const T& v) { return glm::saturate(v); }

template<typename T>
float length(const T& v) { return glm::length(v); }

#define INF 10000000
struct Ray;
struct Rand
{
    Rand(unsigned int seed = 0) : norm_unif_rand(0, 1), gen( std::mt19937::default_seed + seed ) { }
    std::uniform_real_distribution<float> norm_unif_rand;
    std::mt19937 gen;
    float next01()
    {
		return norm_unif_rand(gen);
    }
};
struct TestDeterministicRand
{
    TestDeterministicRand() : i(0) { }
    float next01()
    {
        i += 0.3211666f;
        i = (float)fmod(i, 1);
        return i;
    }
    float i;  
};
inline bool equal(float a, float b, float epsilon)
{
	return abs(a-b) < epsilon;
}

namespace zup // z up
{
	inline float cos_theta(const float3& v) { return v.z; }
	inline float abs_cos_theta(const float3& v) { return glm::abs(cos_theta(v)); }

	inline float sin_theta2(const float3& v) { return 1.0f - v.z * v.z; }
	inline float sin_theta(const float3& v) 
	{ 
		float temp = sin_theta2(v);
		if (temp <= 0.0f) return 0.0f;
		else return std::sqrt(temp);
	}	
	inline float3 sph2cart(float sintheta, float costheta, float phi)
	{
		return float3(sintheta * cos(phi), sintheta * sin(phi), costheta);
	}
	inline bool same_hemi(const float3& a, const float3& b)
	{
		return a.z * b.z > 0;
	}
};

inline float luminance(float3 rgb)
{
	 return glm::dot(float3(0.2126, 0.7152, 0.0722), rgb);
}
inline void axisConversions(const float3& normal, float3x3* zUpToWorld, float3x3* worldToZUp)
{
	//hopefully we can collapse this..
	if(normal.x == 0 && normal.y == 0 && normal.z == 1)
	{
		*zUpToWorld = float3x3();
		//do nothing
	}
	else if(normal.x == 0 && normal.y == 0 && normal.z == -1)	
	{
		float3 rotaxis(0, 1, 0);
		float rotangle = PI;
		*zUpToWorld = float3x3(glm::rotate(glm::degrees(rotangle), rotaxis));
	}
	else
	{					
		float3 rotaxis = cross(float3(0, 0, 1), normal);
		float rotangle = acos(dot(normal, float3(0, 0, 1)));
		*zUpToWorld = float3x3(glm::rotate(glm::degrees(rotangle), rotaxis));
	}
	*worldToZUp = glm::transpose(*zUpToWorld);	
}
struct ShadingCS
{
	ShadingCS(const float3& normal)
	{
		axisConversions(normal, &zUpToWorld, &worldToZUp);
	}
	float3 local(const float3& globalDir) const { return worldToZUp * globalDir; }
	float3 world(const float3& localDir) const { return zUpToWorld * localDir; }
private:
	float3x3 zUpToWorld;
	float3x3 worldToZUp;
};
inline float3 sampleCosineWeightedHemisphere(Rand& rand, float* pdf)
{
	float rx = rand.next01(); float ry = rand.next01();
	float a = sqrt(1 - ry);
	float3 w = float3(cos(2 * PI * rx) * a, sin(2 * PI * rx) * a, sqrt(ry));
	*pdf = INV_PI * zup::cos_theta(w);
	return w;
}

inline float3 sampleHemisphere(Rand& rand)
{
	float rx = rand.next01(); float ry = rand.next01();
	float a = sqrt(1 - ry * ry);
	float3 w = float3(cos(2 * PI * rx) * a, sin(2 * PI * rx) * a, ry);
	return w;
}