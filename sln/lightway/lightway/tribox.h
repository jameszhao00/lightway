#pragma once
/********************************************************/

/* AABB-triangle overlap test code                      */

/* by Tomas Akenine-Möller                              */

/* Function: int triBoxOverlap(float boxcenter[3],      */

/*          float boxhalfsize[3],float triverts[3][3]); */

/* History:                                             */

/*   2001-03-05: released the code in its first version */

/*   2001-06-18: changed the order of the tests, faster */

/*                                                      */

/* Acknowledgement: Many thanks to Pierre Terdiman for  */

/* suggestions and discussions on how to optimize code. */

/* Thanks to David Hunt for finding a ">="-bug!         */

/********************************************************/

#include <math.h>

#include <stdio.h>


int planeBoxOverlap(float normal[3], float vert[3], float maxbox[3]);



int triBoxOverlap(float boxcenter[3],float boxhalfsize[3],float triverts[3][3]);


#include "shapes.h"
#include <glm/ext.hpp>
inline bool tri_aabb_test(const Triangle& tri, const AABB& aabb)
{
	float tri_verts[3][3] = {
		tri.vertices[0].position.x, 
		tri.vertices[0].position.y, 
		tri.vertices[0].position.z, 
		tri.vertices[1].position.x, 
		tri.vertices[1].position.y, 
		tri.vertices[1].position.z, 
		tri.vertices[2].position.x, 
		tri.vertices[2].position.y, 
		tri.vertices[2].position.z, 
	};

	float3 box_center = aabb.min_pt + aabb.extent / float3(2);
	float3 box_halfsize = aabb.extent / float3(2);
	
	return (bool)triBoxOverlap(glm::value_ptr(box_center), glm::value_ptr(box_halfsize), tri_verts);
}