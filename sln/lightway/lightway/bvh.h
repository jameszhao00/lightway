#pragma once
#include <memory>
#include "math.h"
struct StaticScene;

struct AABB
{
	float3 minPt; float3 maxPt;
	//top down, starts at maxPt
	float3 corner(int idx);
};
AABB unionOf(const AABB& a, const AABB& b);
struct BVHBuildPrimitive
{
	AABB aabb;
	int id0;
};
template<typename T>
struct ArrayView
{
	const T& operator[](int i) const
	{
		return dataPtr[startIdx + i];
	}
	T& operator[](int i)
	{
		return dataPtr[startIdx + i];
	}
	ArrayView subview(int i, int c)
	{
		ArrayView av;
		assert(c <= count);
		assert(i + c <= count);
		assert(c > 0);
		av.startIdx = startIdx + i;
		av.count = c;
		return av;
	}
	int startIdx;
	int count;
	const T* dataPtr;
};
struct BVHNode
{
	enum SplitAxis
	{
		SplitAxisX = 0,
		SplitAxisY,
		SplitAxisZ
	};
	union
	{
		struct  
		{
			BVHNode* children[2];
		} branch;
		struct 
		{

		} leaf;
	};
	bool isLeaf;
	AABB aabb;
	SplitAxis splitAxis;
	void build(const ArrayView<BVHBuildPrimitive>& primitives);
};
std::unique_ptr<BVHNode> makeAccelScene(const StaticScene* scene);