#include "pch.h"
#include "test.h"
#include "RayTracer.h"

void test_rectangular_area_lights()
{
	Rand rand;
	for(int i = 0; i < 1000; i++)
	{
		mat4 t = glm::rotate(rand.next01() * 360.f, normalize(vec3(rand.next01(), rand.next01(), rand.next01()))) * 
			glm::translate(rand.next01(), rand.next01(), rand.next01());
		RectangularAreaLight ral;
		ral.corners[0] = vec3(t * vec4(-1, 0, -1, 1));
		ral.corners[1] = vec3(t * vec4(-1, 0, 1, 1));
		ral.corners[2] = vec3(t * vec4(1, 0, 1, 1));
		ral.corners[3] = vec3(t * vec4(1, 0, -1, 1));
		ral.normal = cross(normalize(ral.corners[1] - ral.corners[0]), normalize(ral.corners[3] - ral.corners[0]));
		
		for(int s = 0; s < 10000; s++)
		{
			vec3 pt = ral.sample_pt(rand);
			vec3 o = vec3(rand.next01(), rand.next01(), rand.next01()) * vec3(10);
			vec3 d = normalize(pt - o);
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