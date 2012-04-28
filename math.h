#pragma once

#include <glm/glm.hpp>
#include <random>
#include <limits>
using namespace glm;

typedef ivec3 int3;
typedef vec3 float3;
typedef ivec4 int4;
typedef vec4 float4;

#define PI 3.1415f
#define INV_PI 1/PI
#define INV_PI_V3 vec3(INV_PI)
#define DEBUGVAL -5000000

#define INF 10000000
struct Ray;
struct Rand
{
    Rand() : norm_unif_rand(0, 1) {  }
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