#pragma once
#include <list>
#include <gl/glfw.h>
#include <boost/thread.hpp>
//#include <mutex>
#include "rendering.h"
#include "shapes.h"

static void report_gl_error(const char* file, int line_number)
{
	GLenum error = glGetError();
	if(error != GL_NO_ERROR)
	{
		printf("ERROR: OGL (%s@%d) : %s\n", file, line_number,
			gluErrorString(error));
	}
}

#define GLE report_gl_error(__FILE__, __LINE__)

using namespace std;

template<typename T>
struct Colored
{
	Colored(const T& o, const float3& c) : object(o), color(c)
	{

	}
	T object;
	float3 color;
};
struct DebugRay : Ray
{
	float t;
};
inline void glvert(float3 vert)
{
	glVertex4f(vert.x, vert.y, vert.z, 1);
}
class DebugDraw
{
public:
	void init();
	void setup_gl();
	void restore_gl();
	void flip();
	void add_sphere(const Sphere& sphere, float3 color = float3(1));
	void add_ray(const Ray& ray, float length = -1, float3 color = float3(1));
	void add_ray(const float3& origin, const float3& dir, float length = -1, float3 color = float3(1));
	void add_aabb(const AABB& aabb, float3 color = float3(1))
	{
		aabbs[current].push_back(Colored<AABB>(aabb, color));
	}
	void add_tri(const Triangle& tri, float3 color = float3(1));
	void add_disc( const Disc& disc, float3 color = float3(1) );
	void draw();
	Camera camera;
	GLUquadric* quad;

	list<Colored<Sphere> > spheres[2];
	list<Colored<DebugRay> > rays[2];
	list<Colored<Disc> > discs[2];
	list<Colored<Triangle>> triangles[2];
	list<Colored<AABB>> aabbs[2];
	int current;
	boost::mutex flip_lock;
private:
	void draw_ray(const DebugRay& ray, float3 color = float3(1));
	void draw_sphere(const Sphere& sphere, float3 color = float3(1));
	void draw_disc(const Disc& sphere, float3 color = float3(1));
	void draw_tri(const Triangle& tri, float3 color = float3(1));
	void draw_aabb(const AABB& aabb, float3 color = float3(1))
	{ 
		glLineWidth(1); GLE;
		glBegin(GL_LINES);
		{
			glColor4f(.2, .2, .2, 1);
			float3 v[] = {
				float3(aabb.min_pt.x, aabb.min_pt.y, aabb.min_pt.z),
				float3(aabb.min_pt.x, aabb.min_pt.y, aabb.max_pt.z), //1

				float3(aabb.min_pt.x, aabb.max_pt.y, aabb.min_pt.z), //2
				float3(aabb.min_pt.x, aabb.max_pt.y, aabb.max_pt.z),

				float3(aabb.max_pt.x, aabb.min_pt.y, aabb.min_pt.z), //4
				float3(aabb.max_pt.x, aabb.min_pt.y, aabb.max_pt.z),

				float3(aabb.max_pt.x, aabb.max_pt.y, aabb.min_pt.z), 
				float3(aabb.max_pt.x, aabb.max_pt.y, aabb.max_pt.z), //7
			};
			
			glvert(v[0]); glvert(v[1]);
			glvert(v[0]); glvert(v[2]);
			glvert(v[0]); glvert(v[4]);
			
			glvert(v[7]); glvert(v[3]);
			glvert(v[7]); glvert(v[5]);
			glvert(v[7]); glvert(v[6]);
			
			glvert(v[2]); glvert(v[3]);
			glvert(v[1]); glvert(v[3]);

			glvert(v[2]); glvert(v[6]);
			glvert(v[4]); glvert(v[6]);

			glvert(v[4]); glvert(v[5]);
			glvert(v[1]); glvert(v[5]);
			glColor4f(1,1,1,1);
		}
		glEnd();
	}
};