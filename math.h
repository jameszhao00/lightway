#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <random>
using namespace glm;
#define PI 3.1415f
#define INV_PI 1/PI
#define INV_PI_V3 vec3(INV_PI)
#define DEBUGVAL -5000000
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