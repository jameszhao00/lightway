#include "stdafx.h"
#include "test.h"
#include "RenderCore.h"

void test_rectangular_area_lights()
{
	Rand rand;
	for(int i = 0; i < 1000; i++)
	{
		float4x4 t = glm::rotate(rand.next01() * 360.f, normalize(float3(rand.next01(), rand.next01(), rand.next01()))) * 
			glm::translate(rand.next01(), rand.next01(), rand.next01());
		RectangularAreaLight ral;
		ral.corners[0] = float3(t * float4(-1, 0, -1, 1));
		ral.corners[1] = float3(t * float4(-1, 0, 1, 1));
		ral.corners[2] = float3(t * float4(1, 0, 1, 1));
		ral.corners[3] = float3(t * float4(1, 0, -1, 1));
		ral.normal = cross(normalize(ral.corners[1] - ral.corners[0]), normalize(ral.corners[3] - ral.corners[0]));
		
		for(int s = 0; s < 10000; s++)
		{
			float3 pt = ral.sample_pt(rand);
			float3 o = float3(rand.next01(), rand.next01(), rand.next01()) * float3(10);
			float3 d = normalize(pt - o);
			float ndotd = dot(ral.normal, d);
			if(fabs(ndotd) < 0.0001) continue;
			Ray ray (o, d);
			Intersection i;
			bool hit = ral.intersect(ray, &i, ndotd > 0);
			if(!hit)
			{
				//break here
				ral.intersect(ray, &i, ndotd > 0);
				lwassert(false);
			}
		}		
	}
}