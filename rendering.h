#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
using namespace glm;
struct Material;
struct Camera
{
    Camera() : fovy(45), zn(1), zf(300), eye(vec3(0, 1, 5)), forward(vec3(0, 0, -1)), up(vec3(0, 1, 0)), mouse_pos(ivec2(-1, -1)) { }
	mat4 projection() const
	{
		return perspective(45.0f, ar, zn, zf);
	}
	mat4 view() const
	{
		return lookAt(eye, eye + forward, up);
	}
	float ar;
	float zn, zf;
	vec3 eye;
	vec3 forward;
	vec3 up;
	float fovy;

	void on_mouse_up()
	{
		mouse_pos = ivec2(-1, -1); 
	}
	void on_mouse_move(ivec2 new_pos)
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
	void on_keyboard_event(char key)
	{
		vec3 left = cross(up, forward);
		float scale = 0.01f;
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
	ivec2 mouse_pos;
};
struct Intersection
{
	vec3 position;
	vec3 normal;
	bool hit;
	const Material* material;
	float t;
};
int closest_intersection(const Intersection* intersections, int num_intersections);
