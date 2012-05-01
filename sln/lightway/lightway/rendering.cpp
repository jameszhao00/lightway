#include "stdafx.h"
#include <glm/ext.hpp>
#include "rendering.h"
#include "lwmath.h"
//deliberately 2 versions
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
int closest_intersection(const Intersection* intersections, int num_intersections, float max_t)
{
    int closest_i = -1;
    const Intersection* closest = nullptr;
    for(int i = 0; i < num_intersections; i++)
    {
    	if(intersections[i].hit)
    	{
			bool t_in_bounds = intersections[i].t < max_t;

			if(((closest_i == -1) || (intersections[i].t < closest->t)) && t_in_bounds)
    		{
    			closest_i = i;
    			closest = &intersections[i];
    		}
    	}
    }
    return closest_i;    
}

Camera::Camera() : stateIdx(0), fovy(45), zn(1), zf(300), eye(float3(2, 0, 0)), forward(float3(-1, 0, 0)), up(float3(0, 1, 0)), mouse_pos(int2(-1, -1)) { }
float4x4 Camera::projection() const
{
	return glm::perspective(45.0f, ar, zn, zf);
}
float4x4 Camera::view() const
{
	return glm::lookAt(eye, eye + forward, up);
}

void Camera::on_mouse_up()
{
	mouse_pos = int2(-1, -1); 
}
void Camera::on_mouse_move(int2 new_pos)
{
	if(mouse_pos.x != -1)
	{
		int2 dp = new_pos - mouse_pos;
		float3 right = cross(up, forward);
		float4x4 p_rot = glm::rotate((float)dp.y * .2f, right);
		float4x4 h_rot = glm::rotate(-(float)dp.x * .2f, up);
		float4x4 rot = p_rot * h_rot;
		forward = float3(rot * float4(forward, 0));
		up = float3(rot * float4(up, 0));
		stateIdx++;
	}
	mouse_pos = new_pos;
}
void Camera::on_keyboard_event(char key)
{
	float3 left = cross(up, forward);
	float scale = 0.008f;
	if(key == 'W')
	{
		eye += scale * forward;
		stateIdx++;
	}
	else if(key == 'A')
	{
		eye += scale * left;
		stateIdx++;
	}
	else if(key == 'S')
	{
		eye += scale * -forward;
		stateIdx++;
	}
	else if(key == 'D')
	{
		eye += scale * -left;
		stateIdx++;
	}
}