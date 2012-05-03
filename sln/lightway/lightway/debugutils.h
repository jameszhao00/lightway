#pragma once
#include <cstdio>
#include <glm/glm.hpp>
#include "lwmath.h"
#ifdef LWFAST

inline void print_float3(float3 a)
{
}
inline void lwassert_allgreater(float3 a, float3 b)
{
}
//greater than or equal to
inline void lwassert_allge(float3 a, float3 b, int line_num = -1)
{
}
#define LWASSERT_ALLGE(X, Y) ((void)0)
inline void lwassert_greater(float a, float b, int line_num = -1)
{
}
inline void lwassert_less(float a, float b, int line_num = -1)
{
}
inline void lwassert_validfloat(float v, int line_num = -1) 
{
}
inline void lwassert_validvec(float3 v, int line_num = -1)
{
}
#define LWASSERT_VALIDFLOAT(X) ((void)0)
#define LWASSERT_VALIDVEC(X) ((void)0)
template<typename T>
inline void lwassert(T b)
{
}

inline void lwassert_notequal(float3 a, float3 b)
{
}
#else
namespace du
{
	template<typename T>
	bool isnan(T a) { return a != a; }
	inline bool isinf(float a) 
	{
		return (a < -FLT_MAX) || (a > FLT_MAX); 
	}
}
inline void print_float3(float3 a)
{
	printf("{%f, %f, %f}", a.x, a.y, a.z);
}
inline void lwassert_allgreater(float3 a, float3 b)
{
	if((a.x <= b.x) || (a.y <= b.y) || (a.z <= b.z))
	{
		printf("assert failed: ");
		print_float3(a);
		printf(" > ");
		print_float3(b);
		printf("\n");
		__debugbreak();
	}
}
//greater than or equal to
inline void lwassert_allge(float3 a, float3 b, int line_num = -1)
{
	if((a.x < b.x) || (a.y < b.y) || (a.z < b.z))
	{
		printf("(L%d) assert failed: ", line_num);
		print_float3(a);
		printf(" >= ");
		print_float3(b);
		printf("\n");
		__debugbreak();
	}
}
#define LWASSERT_ALLGE(X, Y) lwassert_allge((X), (Y), __LINE__)
inline void lwassert_greater(float a, float b, int line_num = -1)
{
	if(!(a > b)) 
	{
		printf("(L%d) assert failed: %f > %f\n", line_num, a, b);
		__debugbreak();
	}
}
inline void lwassert_less(float a, float b, int line_num = -1)
{
	if(!(a < b)) 
	{
		printf("(L%d) assert failed: %f < %f\n", line_num, a, b);
		__debugbreak();
	}
}
inline void lwassert_validfloat(float v, int line_num = -1) 
{
	if(du::isnan(v)) 
	{ 
		printf("(L%d) float is NAN", line_num); 
		__debugbreak();
	} 
	if(du::isinf(v)) 
	{ 
		printf("(L%d) float is INF", line_num); 
		__debugbreak();
	} 
}
inline void lwassert_validvec(float3 v, int line_num = -1)
{
	if(du::isnan(v.x) || du::isnan(v.y) || du::isnan(v.z)) 
	{
		printf("(L%d) float3 is NAN", line_num); 
		__debugbreak(); 
	}
	if(du::isinf(v.x) || du::isinf(v.y) || du::isinf(v.z)) 
	{ 
		printf("(L%d) float3 is INF", line_num); 
		__debugbreak(); 
	}
}
#define LWASSERT_VALIDVEC(X) lwassert_validvec((X), __LINE__)
#define LWASSERT_VALIDFLOAT(X) lwassert_validfloat((X), __LINE__)
template<typename T>
inline void lwassert(T b)
{
	if(!(bool)b)
	{
		printf("assert failed\n"); 
		__debugbreak(); 
	}
}

inline void lwassert_notequal(float3 a, float3 b)
{
	if(a.x == b.x && a.y == b.y && a.z == b.z)
	{
		printf("assert failed\n"); 
		__debugbreak(); 
	}
}
#endif