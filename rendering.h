#pragma once
#include "math.h"
using namespace glm;
struct Material;
struct Camera
{
    Camera();
	mat4 projection() const;
	mat4 view() const;
	float ar;
	float zn, zf;
	vec3 eye;
	vec3 forward;
	vec3 up;
	float fovy;

	void on_mouse_up();
	void on_mouse_move(ivec2 new_pos);
	void on_keyboard_event(char key);
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
int closest_intersection(const Intersection* intersections, int num_intersections, float max_t);
