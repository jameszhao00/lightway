#pragma once
#include <list>
#include <gl/glfw.h>
#include <mutex>
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
	Colored(const T& o, const vec3& c) : object(o), color(c)
	{

	}
	T object;
	vec3 color;
};
struct DebugRay : Ray
{
    float t;
};
class DebugDraw
{
public:
	void init();
	void setup_gl();
	void restore_gl();
	void flip();
	void add_sphere(const Sphere& sphere, vec3 color = vec3(1));
	void add_ray(const Ray& ray, float length = -1, vec3 color = vec3(1));
	void add_ray(const vec3& origin, const vec3& dir, float length = -1, vec3 color = vec3(1));
	
	void add_disc( const Disc& disc, vec3 color = vec3(1) );
	void draw();
	Camera camera;
	GLUquadric* quad;

	list<Colored<Sphere> > spheres[2];
	list<Colored<DebugRay> > rays[2];
	list<Colored<Disc> > discs[2];
	int current;
	
    mutex flip_lock;
private:
	void draw_ray(const DebugRay& ray, vec3 color = vec3(1));
	void draw_sphere(const Sphere& sphere, vec3 color = vec3(1));
	void draw_disc(const Disc& sphere, vec3 color = vec3(1));
};
