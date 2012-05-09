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
	Intersection() : position(INF), normal(INF), lightId(-1), hit(false), material(nullptr), t(INF) { }	
	/*
	//if it matters later on...
	struct GenericIntersection
	{
		float3 position;
		float3 normal;
		float t;
		bool hit;
	};
	struct : public GenericIntersection
	{
		float3 shadingNormal;
		int primitiveId;
		const Material* material;
		float2 uv;
	} scene;
	struct : public GenericIntersection
	{
		int lightId;
		float3 emission;
	} light;
	*/
	float3 position;
	float3 shadingNormal;
	float3 normal;
	int lightId;
	int primitiveId;
	bool hit;
	const Material* material;
	float t;
	float2 uv;
	
};
int closest_intersection(const Intersection* intersections, int num_intersections);
int closest_intersection(const Intersection* intersections, int num_intersections, float max_t);
