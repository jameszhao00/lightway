#pragma once
#include "uniformgrid.h"
#include "scene.h"
#include <iostream>
#include <set>
using namespace std;
#include "shapes.h"
#include "test.h"
#include "tribox.h"
void test_traversal()
{return;
	
	{
		UniformGrid ug(ivec3(5), AABB(vec3(0), vec3(5)));
		/*
		{
			//test negative directions
			Ray ray(vec3(6.01, 6.01, 6), normalize(vec3(-1, -1, -1)));
			vec3 t_delta, step, t_max;
			ivec3 cellid; float outside;
			ug.dda_vals(ray, &t_delta, &step, &t_max, &cellid, &outside);
			cout << t_delta << step << t_max << cellid << outside;
			for(int i = 0; i < 12; i++)
			{			
				bool hit = ug.dda_next(t_delta, step, outside, &t_max, &cellid);
			
				cout << i << " " << vec3(cellid).x << vec3(cellid).y << vec3(cellid).z  << endl;
				if(!hit)
				{
					cout << "not hit!" << endl;
				}
				assert(hit);
			}
			bool hit = ug.dda_next(t_delta, step, outside, &t_max, &cellid);
			assert(!hit);
		}
		{
			Ray ray(vec3(1, 2, 0), normalize(vec3(1, 0, 0)));
			vec3 t_delta, step, t_max;
			ivec3 start_cell, outside;
			//assert(ug.dda_vals(ray, &t_delta, &step, &t_max, &start_cell, &outside));
			assert(t_delta.x == 1);
			assert(step.x == 1);
			assert(t_max.x == 1);
			assert(equal3(start_cell, ivec3(1, 2, 0)));
			assert(equal3(outside, ivec3(5, 2, 0)));
		}
		{
			//test dda_vals
			Ray ray(vec3(-1, 2, 0), normalize(vec3(1, 1, 0)));
			vec3 t_delta, step, t_max;
			ivec3 cellid; float outside;
			assert(ug.dda_vals(ray, &t_delta, &step, &t_max, &cellid, &outside));
		
			assert(t_delta.x == 1/ray.dir.x && t_delta.y == 1/ray.dir.y);
			assert(step.x == 1 && step.y == 1);
			assert(equal3(cellid, ivec3(0, 3, 0)));
		}
		{
			//test complete miss
			Ray ray(vec3(-1, 2, 0), normalize(vec3(0, 1, 0)));
			vec3 t_delta, step, t_max;
			ivec3 cellid; float outside;
			assert(!ug.dda_vals(ray, &t_delta, &step, &t_max, &cellid, &outside));
		
		}
	
		{
			//test each individual hit
			Ray ray(vec3(-1, 2, 0), normalize(vec3(1, 0, 0)));
			vec3 t_delta, step, t_max;
			ivec3 cellid; float outside;
			assert(ug.dda_vals(ray, &t_delta, &step, &t_max, &cellid, &outside));
		
			bool hit = ug.dda_next(t_delta, step, outside, &t_max, &cellid);
			assert(hit);
			assert(glm::all(glm::equal(cellid, ivec3(1, 2, 0))));
			hit = ug.dda_next(t_delta, step, outside, &t_max, &cellid);
			assert(hit);
			assert(glm::all(glm::equal(cellid, ivec3(2, 2, 0))));
			hit = ug.dda_next(t_delta, step, outside, &t_max, &cellid);
			assert(hit);
			assert(glm::all(glm::equal(cellid, ivec3(3, 2, 0))));
			hit = ug.dda_next(t_delta, step, outside, &t_max, &cellid);
			assert(hit);
			assert(glm::all(glm::equal(cellid, ivec3(4, 2, 0))));
			hit = ug.dda_next(t_delta, step, outside, &t_max, &cellid);
			assert(!hit);
		}
		{
			//test full traversal
			Ray ray(vec3(0, 0.1, 0), normalize(vec3(1, 1, 0)));
			vec3 t_delta, step, t_max;
			ivec3 cellid; float outside;
			assert(ug.dda_vals(ray, &t_delta, &step, &t_max, &cellid, &outside));
			for(int i = 0; i < 8; i++)
			{			
				bool hit = ug.dda_next(t_delta, step, outside, &t_max, &cellid);
				assert(hit);
			}
			bool hit = ug.dda_next(t_delta, step, outside, &t_max, &cellid);
			assert(!hit);
		}
	
	
		{
			//test box tri intersection
			Triangle tri(vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 1, 0), vec3(0), nullptr);
			AABB aabb(vec3(-10, -10, -10), vec3(.01, 0.01,.01));
			bool result = tri_aabb_test(tri, aabb);
			assert(result);
		}
	
		{
			//test with scene
		
			auto scene = load_scene("assets/bunny.obj", vec3(0, 0.02, 0), 10.f);
			auto ug = make_uniform_grid(*scene, ivec3(10, 10, 10));
			int total = 0;
			int n = 0;
			for(int x = 0; x < ug->subdivisions.x; x++)
			{
				for(int y = 0; y < ug->subdivisions.y; y++)
				{
					for(int z = 0; z < ug->subdivisions.z; z++)
					{
						//cout <<x <<", " << y << ", " << z << " = " << ug->cell(ivec3(x, y, z))->count << endl;
						int c = ug->cell(ivec3(x, y, z))->count;
						if(c > 0)
						{
							total += c;
							n += 1;
						}
					}
				}
			}
			cout <<(float)total/ n;
		}*/
		//test are all tris accounted for?
		{
			/*
			auto scene = load_scene("assets/bunny.obj", vec3(0, 0.02, 0), 10.f);
			cout << "original scene has " << scene->triangles.size() << " tris " << endl;
			for(int i = 10; i < 60; i+=10)
			{
				std::set<const Triangle*> tris;
				auto ug = make_uniform_grid(*scene, ivec3(10, 10, 10));
				for(int x = 0; x < ug->subdivisions.x; x++)
				{
					for(int y = 0; y < ug->subdivisions.y; y++)
					{
						for(int z = 0; z < ug->subdivisions.z; z++)
						{
							auto cell = ug->cell(ivec3(x, y, z));
							int c = cell->count;
							if(c > 0)
							{
								for(int tri_i = 0; tri_i < c; tri_i++)
								{
									tris.insert(scell->data[tri_i]);
								}
							}
						}
					}
				}
				cout << "subd " << i <<" has " << tris.size() << " unique tris" << endl;
				assert(tris.size() == scene->triangles.size());
			}
			*/
		}
	}
		
}
void test_uniform_grid()
{
	test_traversal();
}