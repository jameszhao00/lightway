#pragma once
#include "lwmath.h"

template<typename T>
bool equal3(T a, T b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}
template<typename T>
bool equal4(T a, T b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

template<typename T>
bool greater3(T a, T b)
{
	return a.x > b.x && a.y > b.y && a.z > b.z;
}
template<typename T>
bool less3(T a, T b)
{
	return a.x < b.x && a.y < b.y && a.z < b.z;
}
void test_rectangular_area_lights();
void testUniformGridAggregate();