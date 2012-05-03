#pragma once
#include "lwmath.h"
#include <iostream>
using namespace std;

float blinnphong_pdf(float3 h, float3 v, float3 normal, float exponent)
{
    float costheta = dot(h, normal);
    float vdoth = dot(v, h);
    return (exponent+1)*pow(costheta, exponent)/(2*PI*4*vdoth);
}
template<typename TRand>
void sample_blinnphong(TRand& rand, int num_dirs, const float3* v, 
    const float* n, float3* buf, float* prob)
{
    for(int i = 0; i < num_dirs; i++)
    {
		float r1 = rand.next01();
		float r2 = rand.next01();
		float costheta = powf(r1, 1.f / (n[i]+1));
		float sintheta = sqrt(glm::max(0.f, 1.f - costheta*costheta));
        float phi = r2 * 2 * PI;
        //printf("ct, st, phi = %f, %f, %f\n", costheta, sintheta, phi);
        float3 h;
        //sph2cart(sintheta, costheta, phi, &h);

        sph2cart(sintheta, costheta, phi, &h);
        
        //printf("xyz = %f, %f, %f\n", h.x, h.y, h.z);
        //printf("phi = %f\n", sin(phi));
        buf[i] = h;
		//dot(o, h) should be equal to dot(i, h)
        float prob_h = (n[i] + 1)*pow(costheta, n[i])/(2*PI*4*dot(v[i], h));
        prob[i] = prob_h;
    }
}
float lambert_pdf(float3 i, float3 n)
{
    return dot(i, n) / PI;
}
void sample_lambert(Rand& rand, int num_dirs, float3* buf, float* prob)
{
	for(int i = 0; i < num_dirs; i++)
	{
		float r1 = rand.next01();
		float r2 = rand.next01();

		float a = sqrt(1 - r2);

		float x = cos(2 * PI * r1) * a;
		float y = sin(2 * PI * r1) * a;
		float z = sqrt(r2);

		float phi = 2 * PI * r1;
		float theta = acos(sqrt(r2));

		float p = (float)(cos(theta) / PI);
		buf[i] = float3(x, y, z);
		prob[i] = p;
	}
}
//hemisphere
void sample_hs(Rand& rand, const float3& n, int count, float3* dir)
{    
    for(int i = 0; i < count; i++)
    {
        float r1 = rand.next01();      
        float r2 = rand.next01();
		float a = sqrt(r2 * (1 - r2));
        
        float x = 2 * cos(2 * PI * r1) * a;
        float y = 2 * sin(2 * PI * r1) * a;
        float z = 1 - 2 * r2;
        
        float3 d(x, y, z);
        
        if(dot(d, n) < 0) d *= float3(-1);
        
        dir[i] = d;
    }
}