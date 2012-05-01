#pragma once
#include "stdafx.h"
//#include "lightway.h"
#include <QtGui/QApplication>
#include <QThread>
#include <QtOpenGL>
#include <cstdio>
#include <iostream>
#include <list>
#include "RayTracer.h"
using namespace std;
#define INVALID_TEXTURE 1000000


class RenderSlave : public QObject
{
	 Q_OBJECT
public:
	RenderSlave(int myGroup) : myGroup_(myGroup), cameraStateIdx(-1)
	{ 
	} 
    
signals:
	void traceStepFinished(int);
	//not used for now...
	void traceStepUpdate(int, float);
public slots:
	void traceStep(RayTracer* rt, int desiredGroup, int groupN, int w, int h, SampleDebugger* sd)
	{		
		if(desiredGroup != myGroup_) return;
		rt->resize(w, h);
		bool clear = cameraStateIdx != rt->camera.stateIdx;		
		cameraStateIdx = rt->camera.stateIdx;
		rt->raytrace(groupN, myGroup_, groupN, clear, *sd);
		emit traceStepFinished(myGroup_);
	}
private:
	int cameraStateIdx;
	int myGroup_;
};