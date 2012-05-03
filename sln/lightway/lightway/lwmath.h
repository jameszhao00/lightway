#pragma once

#include <glm/glm.hpp>
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

/*
#include <Eigen/Dense>

using namespace Eigen;
float3 toGlm(Vector3f v)
{
	return float3(v.x(), v.y(), v.z());
}
Vector3f toEigen(float3 v)
{
	return Vector3f(v.x, v.y, v.z);
}*/