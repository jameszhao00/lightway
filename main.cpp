#include <cstdio>
#include <iostream>
#include <list>
#include <gl/glfw.h>
#include "RayTracer.h"
#include "debug.h"
#include <thread>
#include <functional>
#include <chrono>
#define GLFW_CDECL
#include <AntTweakBar.h>
using namespace std;


#define INVALID_TEXTURE 1000000
void glfw_mousepos(int a, int b)
{
	TwEventMousePosGLFW(a, b);
}
void glfw_mousebutton(int a, int b)
{
	TwEventMouseButtonGLFW(a, b);
}
void glfw_mousewheel(int a)
{
	TwEventMouseWheelGLFW(a);
}
void glfw_key(int a, int b)
{
	TwEventKeyGLFW(a, b);
}
void glfw_char(int a, int b)
{
	TwEventCharGLFW(a, b);
}
class Program
{
public:
    void init()
    {        
        w = -1; h = -1;
        texture = INVALID_TEXTURE;
        glfwInit();
		mouse_pos = ivec2(-1, -1);
    	glfwOpenWindow(800, 400, 8, 8, 8, 8, 8, 8, GLFW_WINDOW);
    	glDisable(GL_DEPTH_TEST); GLE;
    	glDisable(GL_CULL_FACE); GLE;
    	glEnable( GL_TEXTURE_2D ); GLE;
    	
        running = false;
        
        hw_conc = thread::hardware_concurrency() - 1;
		int r = TwInit(TW_OPENGL, NULL);

		
		glfwSetMouseButtonCallback(glfw_mousebutton);
		glfwSetMousePosCallback(glfw_mousepos);
		glfwSetMouseWheelCallback(glfw_mousewheel);
		glfwSetKeyCallback(glfw_key);
		glfwSetCharCallback(glfw_char);
		rt.init();
	}
	void resize(int nw, int nh)
	{
		w = nw; h = nh;
		int rt_w = w/2; int rt_h = h;
		rt.resize(rt_w, rt_h);
		rt.camera.ar = (float)rt_w/rt_h;
		for(int i = 0; i < hw_conc; i++) dd[i].camera.ar = (float)rt_w/rt_h;
		if(texture != INVALID_TEXTURE)
		{
			glDeleteTextures(1, &texture); GLE;
		}
		
		printf("texture = %d\n", texture);
		glGenTextures(1, &texture); GLE;
		
		printf("texture = %d\n", texture);
		glBindTexture(GL_TEXTURE_2D, texture); GLE;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); GLE;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); GLE;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); GLE;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); GLE;

		TwWindowSize(w, h);
		glMatrixMode(GL_PROJECTION); GLE;
		glLoadIdentity(); GLE;

		glMatrixMode(GL_MODELVIEW); GLE;
		glLoadIdentity(); GLE;
	}
	void check_window_size()
	{
		int nw; int nh;
		glfwGetWindowSize(&nw, &nh);
		if(nw != w || nh != h)
		{
			resize(nw, nh);
			clear_fb();
		}		
	}
	void check_mouse_movement()
	{
		int x; int y;
		glfwGetMousePos(&x, &y);
		ivec2 new_mouse_pos(x, y);
		if(mouse_pos.x != -1 && ((new_mouse_pos.x != mouse_pos.x) || (new_mouse_pos.y != mouse_pos.y)))
		{
			if(glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT))
			{
				if(x < w/2)
				{
				    rt.camera.on_mouse_move(new_mouse_pos);
				    clear_fb();
			    }
				else
				{
				    for(int i = 0; i < hw_conc; i++) dd[i].camera.on_mouse_move(new_mouse_pos);
			    }
			}
		}
		mouse_pos = new_mouse_pos;
	}
	void check_mouse_button()
	{		
		if(glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
		{
			rt.camera.on_mouse_up();
			for(int i = 0; i < hw_conc; i++) dd[i].camera.on_mouse_up();
		}
		if(glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
		{
        	int x; int y;
    		glfwGetMousePos(&x, &y);
            if(x < w/2)
            {
                clear_fb();
                rt.debug_pixel = ivec2(x, h - y);
                //printf("setting debug pixel to %d, %d\n", rt.debug_pixel.x, rt.debug_pixel.y);
            }
		}
	}
    void check_keyboard()
    {
		char keys[] = {'W', 'A', 'S', 'D'};

		int x, y;
		glfwGetMousePos(&x, &y);

		for(int i = 0; i < 4; i++)
		{
			char key = keys[i];
			if(glfwGetKey(key) == GLFW_PRESS)
			{
				if(x < w/2)
				{
				    rt.camera.on_keyboard_event(key);
                    clear_fb();
			    }
				else
				{
                    for(int i = 0; i < hw_conc; i++) dd[i].camera.on_keyboard_event(key);
			    }
			}
		}
    }
    void clear_fb()
    {
        for(int i = 0; i < hw_conc; i++) clear_flag[i] = true;
    }
    void destroy()
    {        
        glfwTerminate();
    }

	void display(void)
	{


		int rt_w = w/2; int rt_h = h;
		glBindTexture(GL_TEXTURE_2D, texture); GLE;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rt_w, rt_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rt.fb); GLE;
		
		glViewport(0, 0, rt_w, rt_h); GLE;
		glBegin( GL_QUADS );
		{	
			glTexCoord2f(0, 0);
			glVertex4f(-1, -1, 0, 1);
			
			glTexCoord2f(1, 0);
			glVertex4f(1, -1, 0, 1);
			
			glTexCoord2f(1, 1);
			glVertex4f(1, 1, 0, 1);
			
			glTexCoord2f(0, 1);
			glVertex4f(-1, 1, 0, 1);		
		}
		glEnd(); GLE;
		glViewport(0, 0, w, h); GLE;
	}
    void main_loop()
    {            


        running = true;
        printf("creating %d threads...\n", hw_conc);
		check_window_size(); //init rt's sizes
        for(int i = 0; i < hw_conc; i++)
        {         
            dd[i].init();
            raytrace_threads[i] = thread(&Program::raytrace_loop, this, hw_conc, i);   
        }
    	while(running)
    	{
			check_window_size();
			check_mouse_movement();
			check_mouse_button();
			check_keyboard();
    		glClearDepth(1); GLE;
    		glClearColor(0, 0, 0, 0); GLE;
    		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); GLE;
    		display();
			
            glViewport(w/2, 0, w/2, h);
			
            for(int i = 0; i < hw_conc; i++)
            {
                
        		dd[i].setup_gl();
        		dd[i].draw();
        		dd[i].restore_gl();
            }
			
    		glViewport(0, 0, w, h);
			

			TwDraw();
    		glfwSwapBuffers(); GLE;
            running = !glfwGetKey( GLFW_KEY_ESC ) && glfwGetWindowParam( GLFW_OPENED );
            
            float avg_krays_per_sec = 0;
            for(int i = 0; i < hw_conc; i++)
            {
                avg_krays_per_sec += krays_per_sec[i];
            }
            //can average even though rays / group my not be equal (they're very close)
            avg_krays_per_sec /= hw_conc;
            //printf("perfstat - %f krays / sec\n", avg_krays_per_sec);
    	}
        for(int i = 0; i < hw_conc; i++)
        {
            raytrace_threads[i].join();
        }
    }
    void raytrace_loop(int total_groups, int my_group)
    {
		int i = 1;
        int frame = 0;
		auto start = chrono::high_resolution_clock::now();
		int total_rays = 0;
		int max_samples = 10;
        while(running)
		{     
			//if(frame == max_samples) break;
		    //not really thread safe... but w/e
            bool clear = clear_flag[my_group];
            if(clear) clear_flag[my_group] = false;
            
			total_rays += rt.raytrace(dd[my_group], total_groups, my_group, hw_conc, clear);
			if(i % 10 == 0)
			{				
				auto end = chrono::high_resolution_clock::now();
				double ratio = (double)chrono::high_resolution_clock::period::num /  chrono::high_resolution_clock::period::den;
				double s =  ((end - start) * ratio).count();
                double rays_per_sec = total_rays / s;
                krays_per_sec[my_group] = rays_per_sec / 1000;
                
				start = end;
				total_rays = 0;
			}
			i++;
            frame++;
		}
	}    
    bool clear_flag[32];
    int hw_conc;
	float krays_per_sec[32];
    thread raytrace_threads[32];
    bool running;
	GLuint texture;
	RayTracer rt;
	DebugDraw dd[32];
	int w; int h;
	ivec2 mouse_pos;
};
int main(int argc, char* argv[])
{
    Program prog;
    prog.init();
    prog.main_loop();
	return EXIT_SUCCESS;
}
