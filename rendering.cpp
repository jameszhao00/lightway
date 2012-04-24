#include "rendering.h"
#include "shading/material.h"
#include "math.h"
int closest_intersection(const Intersection* intersections, int num_intersections)
{
    int closest_i = -1;
    const Intersection* closest = nullptr;
    for(int i = 0; i < num_intersections; i++)
    {
    	if(intersections[i].hit)
    	{
    		if((closest_i == -1) || intersections[i].t < closest->t)
    		{
    			closest_i = i;
    			closest = &intersections[i];
    		}
    	}
    }
    return closest_i;    
}