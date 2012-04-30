#pragma once
#include "stdafx.h"
#include "lightway.h"
#include <QtGui/QApplication>
#include <QThread>
#include <QtOpenGL>
#include <cstdio>
#include <iostream>
#include <list>
#include <gl/glfw.h>
#include "RayTracer.h"
#include "debug.h"
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <functional>
#define GLFW_CDECL
#include <AntTweakBar.h>
using namespace std;
#define INVALID_TEXTURE 1000000


class RenderSlave : public QObject
{
	 Q_OBJECT
public:
	RenderSlave(int myGroup) : myGroup_(myGroup), cameraStateIdx(-1)
	{ 
		dd.init();
	} 
    
signals:
	void traceStepFinished(int);
	//not used for now...
	void traceStepUpdate(int, float);
public slots:
	void traceStep(RayTracer* rt, int desiredGroup, int groupN, int w, int h)
	{		
		if(desiredGroup != myGroup_) return;
		rt->resize(w, h);
		bool clear = cameraStateIdx != rt->camera.stateIdx;		
		cameraStateIdx = rt->camera.stateIdx;
		rt->raytrace(dd, groupN, myGroup_, groupN, clear);
		emit traceStepFinished(myGroup_);
	}
private:
	int cameraStateIdx;
	DebugDraw dd;
	int myGroup_;
};