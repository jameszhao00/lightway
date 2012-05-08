#pragma once
#include "stdafx.h"
#include "lightway.h"
#include <QtGui/QApplication>
#include <QtOpenGL>
#include <cstdio>
#include <iostream>
#include <list>
#include "RenderCore.h"
#include <functional>
#include <GL/GLU.h>
using namespace std;
#define INVALID_TEXTURE 1000000

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

class Viewport : public QGLWidget
{
	Q_OBJECT
public:
	Viewport(QWidget* parent = nullptr) : QGLWidget(parent), texture_(INVALID_TEXTURE), renderCore(nullptr) 
	{ 		
		setFocusPolicy(Qt::ClickFocus);
		QTimer* timer = new QTimer();
		timer->setSingleShot(false);
		connect(timer, SIGNAL(timeout()), this, SLOT(update()));
		timer->start(3550);
	}
    void glInit()
    {        
        texture_ = INVALID_TEXTURE;
    	glDisable(GL_DEPTH_TEST); GLE;
    	glDisable(GL_CULL_FACE); GLE;
    	glEnable( GL_TEXTURE_2D ); GLE;		

	}

	void drawFramebuffer(const float4* rgbFb)
	{	
		int w = width(); int h = height();
		
		glMatrixMode(GL_PROJECTION); GLE;
		glLoadIdentity(); GLE;

		glMatrixMode(GL_MODELVIEW); GLE;
		glLoadIdentity(); GLE;
		texture_ = INVALID_TEXTURE;
    	glDisable(GL_DEPTH_TEST); GLE;
    	glDisable(GL_CULL_FACE); GLE;
    	glEnable( GL_TEXTURE_2D ); GLE;	

		if(texture_ == INVALID_TEXTURE)
		{
			glGenTextures(1, &texture_); GLE;		
			glBindTexture(GL_TEXTURE_2D, texture_); GLE;
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); GLE;
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); GLE;
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); GLE;
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); GLE;

		}
		
		
		for(int y = 0; y < h; y++)
		{
			for(int x = 0; x < w; x++)
			{		
				int fb_i = y * w + x;
				//HACK:
				//float3 tonemapped = float3(rgbFb[fb_i]) / (float3(1) + float3(rgbFb[fb_i]));
				float3 tonemapped = float3(rgbFb[fb_i]);
				float3 srgb = glm::pow(tonemapped, float3(1/2.2f));
				
				fbData_[fb_i].r = (int)floor(srgb.x * 255);
				fbData_[fb_i].g = (int)floor(srgb.y * 255);
				fbData_[fb_i].b = (int)floor(srgb.z * 255);
				fbData_[fb_i].a = 255;
			}
		}
		int rt_w = w;//w/2; 
		int rt_h = h;
		glBindTexture(GL_TEXTURE_2D, texture_); GLE;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rt_w, rt_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, fbData_); GLE;
		
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
	}
	void resizeEvent ( QResizeEvent * event )
	{
		renderCore->resize(int2(width(), height()));
	}
	void resizeGL(int nw, int nh)
	{
		glMatrixMode(GL_PROJECTION); GLE;
		glLoadIdentity(); GLE;

		glMatrixMode(GL_MODELVIEW); GLE;
		glLoadIdentity(); GLE;
	}
	RenderCore* renderCore;
	QPoint selectedPoint;
protected:
	
    void paintEvent(QPaintEvent *event)
	{		
		QPainter painter(this);
		painter.beginNativePainting();

		glViewport(0, 0, width(), height()); GLE;
    	glClearDepth(1); GLE;
    	glClearColor(0, 0, 1, 0); GLE;
    	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); GLE;
			
		glViewport(0, 0, width(), height()); GLE;
		if(renderCore != nullptr)
		{
			drawFramebuffer(renderCore->linear_fb);
		}
			
		painter.endNativePainting();
		
		painter.setPen(Qt::red);
		painter.drawLine(selectedPoint - QPoint(5, 0), selectedPoint + QPoint(5, 0));
		painter.drawLine(selectedPoint - QPoint(0, 5), selectedPoint + QPoint(0, 5));
		
		painter.end();
	}
	virtual void keyPressEvent(QKeyEvent* evt) 
	{
		if(evt->key() == Qt::Key_W)
		{
			renderCore->camera.on_keyboard_event('W');
			renderCore->clear();
		}
		else if(evt->key() == Qt::Key_A)
		{
			renderCore->camera.on_keyboard_event('A');
			renderCore->clear();
		}
		else if(evt->key() == Qt::Key_S)
		{
			renderCore->camera.on_keyboard_event('S');
			renderCore->clear();
		}
		else if(evt->key() == Qt::Key_D)
		{
			renderCore->camera.on_keyboard_event('D');
			renderCore->clear();
		}
	}
	virtual void mousePressEvent(QMouseEvent* evt)
	{
		if(evt->buttons() == Qt::LeftButton)
		{
			selectedPoint = evt->pos();
			selectedPoint.setY(height() - selectedPoint.y());
			renderCore->clear();
		}
	}
	virtual void mouseMoveEvent(QMouseEvent* evt)
	{
		if(evt->buttons() == Qt::LeftButton)
		{
			selectedPoint = evt->pos();
			selectedPoint.setY(height() - selectedPoint.y());
		}
		if(evt->buttons() == Qt::RightButton)
		{			
			renderCore->camera.on_mouse_move(int2(evt->pos().x(), evt->pos().y()));
			renderCore->clear();
		}
	}
	virtual void mouseReleaseEvent(QMouseEvent* evt)
	{		
		renderCore->camera.on_mouse_up();

	}
	//this object hiearchy is a giant mess...
private:

	const QGLWidget* qgl_;
	GLuint texture_;
	bool mouseDown_;
	struct Color {
		unsigned char r, g, b, a;
	};
	Color fbData_[FB_CAPACITY * FB_CAPACITY];
};