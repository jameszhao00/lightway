#include "stdafx.h"
#include <random>
#include <iostream>
using namespace std;
#include "test.h"
#include "uniformgrid.h"
void printRay(const Ray& ray)
{
	cout << "Ray o=";
	print_float3(ray.origin);
	cout << " d=";
	print_float3(ray.dir);
	cout << endl;
}
void testUniformGridAggregate()
{
	const int ITERATIONS = 1000;
	const int TESTS_PER_ITERATION = 1000;
	const int MIN_TRIS = 30;
	const int MAX_TRIS = 200;
	const float3 SUBDIVISIONS_MIN(29);
	const float3 SUBDIVISIONS_MAX(31);
	const float3 MAX_DIMENSION(1000);
	const float3 MAX_OFFSET(1000);
    std::uniform_real_distribution<float> norm_unif_rand;
    std::mt19937 gen;

	auto nextSNorm = [&](){ return (norm_unif_rand(gen) - 0.5) * 0.5; };
	auto nextUNorm = [&](){ return (float)norm_unif_rand(gen); };
	auto nextSNorm3 = [&](){ return float3(nextSNorm(), nextSNorm(), nextSNorm()); };
	auto nextUNorm3 = [&](){ return float3(nextUNorm(), nextUNorm(), nextUNorm()); };
	int targetIteration = -1;
	for(int i = 0; i < ITERATIONS; i++)
	{
		cout << "iteration " << i << endl;
		//generate a bunch of triangles
		StaticScene originalScene;
		int triCount = MIN_TRIS + (int)(nextUNorm() * (MAX_TRIS - MIN_TRIS));
		float3 offset = nextSNorm3() * MAX_OFFSET;
		float3 dimension = nextSNorm3() * MAX_DIMENSION;
		for(int triIdx = 0; triIdx < triCount; triIdx++)
		{
			Triangle tri;
			tri.vertices[0].position = offset + dimension * nextSNorm3();
			tri.vertices[1].position = offset + dimension * nextSNorm3();
			tri.vertices[2].position = offset + dimension * nextSNorm3();
			//hack to get tri id in there!
			tri.material = (Material*)triIdx;
			tri.normal = cross(tri.vertices[0].position, tri.vertices[1].position);
			originalScene.triangles.push_back(tri);
		}
		auto uniformGrid = make_uniform_grid(originalScene, 
			int3(SUBDIVISIONS_MIN + (SUBDIVISIONS_MAX - SUBDIVISIONS_MIN) * nextUNorm3()));
		//test againt uniform grid
		int totalHits = 0;
		for(int testIdx = 0; testIdx < TESTS_PER_ITERATION; testIdx++)
		{
			//we can go in any direction
			float3 rayDir = nextSNorm3();
			//randomly zero out directions
			{
				if(nextUNorm() > 0.9) rayDir.x = 0;
				if(nextUNorm() > 0.9) rayDir.y = 0;
				//only zero out z if either x or y aren't zero
				if(nextUNorm() > 0.9 && (rayDir.x != 0 || rayDir.y != 0)) rayDir.z = 0;
				rayDir = glm::normalize(rayDir);
			}
			//generate a ray
			Ray ray(
				offset + float3(1.2) * dimension * nextSNorm3(), //we can start inside or outside
				rayDir 
			);
			//intersect ray using accl.
			Intersection uniformGridIsect;
			bool flipRay = nextUNorm() > 0.5;
			bool uniformGridHit = false;
			if(targetIteration == -1 || i == targetIteration) 
			{
				IntersectionQuery uniformGridQuery(ray, flipRay, true);
				uniformGridHit = uniformGrid->intersect(uniformGridQuery, &uniformGridIsect);
			}
			//intersect ray naively
			bool naiveHit = false;
			Intersection naiveIsect;
			if(targetIteration == -1 || i == targetIteration)
			{
				float hitT = FLT_MAX;
				for(int triIdx = 0; triIdx < triCount; triIdx++)
				{
					Intersection tempIsect;
					ray.intersect_with_triangles(&originalScene.triangles[triIdx], 
						1, &tempIsect);
					if(tempIsect.hit && tempIsect.t < hitT)
					{
						hitT = tempIsect.t;
						naiveHit = true;
						naiveIsect = tempIsect;
					}
				}
			}
			assert(false); //this test WILL FAIL because tri isect doesnt use IntersectionQuery
			//test if naive and accl. intersection results match
			if(targetIteration == -1 || i == targetIteration)
			{
				if(naiveHit != uniformGridHit)
				{
					cout << "Naive Hit doesn't equal UniformGridHit" << endl;
					printRay(ray);
					__debugbreak();
				}
				if(naiveHit) //uniformGridHit is implied to be true here
				{
					totalHits++;
					if(fabs(uniformGridIsect.t - naiveIsect.t) > 0.001) //t mismatch
					{					
						cout << "T Mismatch accl=" << uniformGridIsect.t << " naive=" << naiveIsect.t << endl;
						printRay(ray);

						__debugbreak();
					}
					if(fabs(uniformGridIsect.t - naiveIsect.t) > 0.0001 && // we might have hit different tris if t is really close
						uniformGridIsect.material != naiveIsect.material) //we hijacked the material param for triIdx
					{					
						cout << "Tri Idx Mismatch" << endl;
						__debugbreak();
					}
				}
			}
		}
		cout << "iteration " << i << " had " << totalHits << " hits" << endl;
	}
}