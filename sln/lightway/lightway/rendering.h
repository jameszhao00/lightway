#pragma once
#include "lwmath.h"
struct Material;
struct Camera
{
    Camera();
	float4x4 projection() const;
	float4x4 view() const;
	float ar;
	float zn, zf;
	float3 eye;
	float3 forward;
	float3 up;
	float fovy;
	//each time something happens, stateIdx increments
	int stateIdx;

	void on_mouse_up();
	void on_mouse_move(int2 new_pos);
	void on_keyboard_event(char key);
	int2 mouse_pos;
};
struct Intersection
{
	Intersection() : position(INF), normal(INF), lightIdx(-1), hit(false), material(nullptr), t(INF) { }
	float3 position;
	float3 normal;
	int lightIdx;
	int primitiveId;
	bool hit;
	const Material* material;
	float t;
};
int closest_intersection(const Intersection* intersections, int num_intersections);
int closest_intersection(const Intersection* intersections, int num_intersections, float max_t);
