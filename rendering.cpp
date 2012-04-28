#include "pch.h"
#include <glm/ext.hpp>
#include "rendering.h"
#include "shading/material.h"
#include "math.h"
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

Camera::Camera() : fovy(45), zn(1), zf(300), eye(vec3(0, 1, 5)), forward(vec3(0, 0, -1)), up(vec3(0, 1, 0)), mouse_pos(ivec2(-1, -1)) { }
mat4 Camera::projection() const
{
	return perspective(45.0f, ar, zn, zf);
}
mat4 Camera::view() const
{
	return lookAt(eye, eye + forward, up);
}

void Camera::on_mouse_up()
{
	mouse_pos = ivec2(-1, -1); 
}
void Camera::on_mouse_move(ivec2 new_pos)
{
	if(mouse_pos.x != -1)
	{
		ivec2 dp = new_pos - mouse_pos;
		vec3 right = cross(up, forward);
		mat4 p_rot = rotate((float)dp.y * .2f, right);
		mat4 h_rot = rotate(-(float)dp.x * .2f, up);
		mat4 rot = p_rot * h_rot;
		forward = vec3(rot * vec4(forward, 0));
		up = vec3(rot * vec4(up, 0));
	}
	mouse_pos = new_pos;
}
void Camera::on_keyboard_event(char key)
{
	vec3 left = cross(up, forward);
	float scale = 0.008f;
	if(key == 'W')
	{
		eye += scale * forward;
	}
	else if(key == 'A')
	{
		eye += scale * left;
	}
	else if(key == 'S')
	{
		eye += scale * -forward;
	}
	else if(key == 'D')
	{
		eye += scale * -left;
	}
}