#include "pch.h"
#include "debug.h"
#include <glm/ext.hpp>
void DebugDraw::init()
{
	quad = gluNewQuadric();
	current = 0;
}

void DebugDraw::setup_gl( )
{
	glEnable(GL_DEPTH_TEST); GLE;
	glDepthFunc(GL_LEQUAL); GLE;
	glMatrixMode(GL_MODELVIEW); GLE;
	glLoadIdentity(); GLE;
	gluLookAt(camera.eye.x, camera.eye.y, camera.eye.z, 
		camera.eye.x + camera.forward.x, camera.eye.y + camera.forward.y, camera.eye.z + camera.forward.z,
		camera.up.x, camera.up.y, camera.up.z); GLE;
	glMatrixMode(GL_PROJECTION); GLE;
	glLoadIdentity(); GLE;
	gluPerspective(camera.fovy, camera.ar, 0.001, 50); GLE;

	glDisable( GL_TEXTURE_2D ); GLE;
	
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	//printf("----- up %f, %f, %f\n", camera.up.x, camera.up.y, camera.up.z);
	//printf("pos %f, %f, %f\n", camera.eye.x, camera.eye.y, camera.eye.z);
}

void DebugDraw::restore_gl()
{
	glDisable(GL_DEPTH_TEST); GLE;
		glMatrixMode(GL_MODELVIEW); GLE;
		glLoadIdentity(); GLE;
		glMatrixMode(GL_PROJECTION); GLE;
		glLoadIdentity(); GLE;
		glEnable( GL_TEXTURE_2D ); GLE;
		
		
		glDisable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void DebugDraw::flip()
{
    flip_lock.lock();
	current = (current + 1) % 2;
	spheres[current].clear();
	rays[current].clear();
	discs[current].clear();
	triangles[current].clear();
	
	aabbs[current].clear();
    flip_lock.unlock();
}
void DebugDraw::add_tri(const Triangle& tri, vec3 color)
{
	triangles[current].push_back(Colored<Triangle>(tri, color));
}
void DebugDraw::add_sphere( const Sphere& sphere, vec3 color /*= vec3(1)*/ )
{
	spheres[current].push_back(Colored<Sphere>(sphere, color));
}

void DebugDraw::add_ray( const Ray& ray, float length, vec3 color /*= vec3(1)*/ )
{
    DebugRay r;
    r.origin = ray.origin;
    r.dir = ray.dir;
    r.t = length != -1 ? length : 10;
	rays[current].push_back(Colored<DebugRay>(r, color));
}
void DebugDraw::add_ray(const vec3& origin, const vec3& dir, float length, vec3 color)
{	
    DebugRay r;
    r.origin = origin;
    r.dir = normalize(dir);
    r.t = length != -1 ? length : 10;
	rays[current].push_back(Colored<DebugRay>(r, color));
}
void DebugDraw::add_disc( const Disc& disc, vec3 color /*= vec3(1)*/ )
{
	discs[current].push_back(Colored<Disc>(disc, color));
}
void DebugDraw::draw()
{
	flip_lock.lock();
	int i = (current + 1) % 2;
	
	for(auto it = rays[i].begin(); it != rays[i].end(); it++) draw_ray(it->object, it->color);
	for(auto it = spheres[i].begin(); it != spheres[i].end(); it++) draw_sphere(it->object, it->color);
	for(auto it = discs[i].begin(); it != discs[i].end(); it++) draw_disc(it->object, it->color);
	for(auto it = aabbs[i].begin(); it != aabbs[i].end(); it++) draw_aabb(it->object, it->color);
	
	for(auto it = triangles[i].begin(); it != triangles[i].end(); it++)	draw_tri(it->object, it->color);
	//cout << "drawn " << triangles[i].size() << " tris " << endl;
	
    flip_lock.unlock();
}

void DebugDraw::draw_ray( const DebugRay&  ray, vec3 color /*= vec3(1)*/ )
{
	glDisable (GL_BLEND);
	vec3 end = ray.origin + ray.dir * ray.t;
	glLineWidth(2); GLE;
	glBegin(GL_LINES);
	{
		glColor4f(color.x, color.y, color.z, 1);
		glVertex4f(ray.origin.x, ray.origin.y, ray.origin.z, 1);
		glVertex4f(end.x, end.y, end.z, 1);
	}
	glEnd(); GLE;
	glColor4f(1, 1, 1, 1);
	glEnable (GL_BLEND);
}
void DebugDraw::draw_tri(const Triangle& tri, vec3 color)
{
	bool wireframe = true;
	vec3 center = (tri.vertices[0].position + tri.vertices[1].position + tri.vertices[2].position) / vec3(3);
	vec3 normal_p = center + tri.normal / vec3(20);
	if(!wireframe)
	{
		glLineWidth(2); GLE;
		glBegin(GL_TRIANGLES);
		{
			glColor4f(color.x, color.y, color.z, 1);
			int i = 0;
			glVertex4f(tri.vertices[i].position.x, tri.vertices[i].position.y, tri.vertices[i].position.z, 1);
			i = 1;
			glVertex4f(tri.vertices[i].position.x, tri.vertices[i].position.y, tri.vertices[i].position.z, 1);
			i = 2;
			glVertex4f(tri.vertices[i].position.x, tri.vertices[i].position.y, tri.vertices[i].position.z, 1);
		}
		glEnd(); GLE;
	}
	else
	{
		
		glLineWidth(2); GLE;
		glBegin(GL_LINES);
		{
			//cout << "addr " << tri.vertices + 0 << ", " << tri.vertices + 1 << ", " << tri.vertices + 2 << endl;
			glColor4f(1, 0, 0, 1);

			for(int i = 0; i < 3; i++)
			{
				glVertex4f(tri.vertices[i].position.x, tri.vertices[i].position.y, tri.vertices[i].position.z, 1);
				glVertex4f(tri.vertices[(i+1)%3].position.x, tri.vertices[(i+1)%3].position.y, tri.vertices[(i+1)%3].position.z, 1);
			}

		}
		glEnd(); GLE;
	}
	
	glBegin(GL_LINES);
	{
		glColor4f(1, 0, 1, 1);
		glVertex4f(center.x, center.y, center.z, 1);
		glVertex4f(normal_p.x, normal_p.y, normal_p.z, 1);
	}
	glEnd(); GLE;
	
	
		glColor4f(1, 1, 1, 1);
}

void DebugDraw::draw_sphere( const Sphere& sphere, vec3 color /*= vec3(1)*/ )
{
	glMatrixMode(GL_MODELVIEW); GLE;
	glPushMatrix(); GLE;
	glColor4f(color.x, color.y, color.z, 1);
	glTranslatef(sphere.center.x, sphere.center.y, sphere.center.z); GLE;
	gluSphere(quad, sphere.radius, 32, 32); GLE;
	glPopMatrix(); GLE;
	glColor4f(1, 1, 1, 1);
}

void DebugDraw::draw_disc( const Disc& disc, vec3 color /*= vec3(1)*/ )
{
	glMatrixMode(GL_MODELVIEW); GLE;
	glPushMatrix(); GLE;
	glColor4f(color.x, color.y, color.z, .1);


	 
	vec3 axis = normalize(cross(vec3(0, 1, 0), disc.normal));
	float degree = angle(disc.normal, vec3(0, 1, 0));

	mat4 base_rot = rotate(90.0f, vec3(1, 0, 0));
	mat4 mv = camera.view() * 
		translate(vec3(disc.center)) *
		rotate(degree, axis) *
		base_rot;
	glLoadMatrixf(glm::value_ptr(mv));

	gluDisk(quad, 0, disc.radius, 30, 30);
	glPopMatrix();
	glColor4f(1, 1, 1, 1);
}
