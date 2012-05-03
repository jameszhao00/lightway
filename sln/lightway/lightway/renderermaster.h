#pragma once

#include "renderslave.h"
#include "viewport.h"
#include "asset.h"
/*
class RendererMaster : public QThread
{
	Q_OBJECT
public:
	RendererMaster(Viewport* vp) : viewport(vp) 
	{
		vp->rayTracer = &rayTracer;
		vp->slaves = &slaves;
	}
	void loadScene()
	{
		static_scene = load_scene("../../../assets/validation/diffuse_validation.lwo", float3(0, 0, 0), 1);
		scene.scene = static_scene.get();
		scene.make_accl();
	}
	void init()
	{
		rayTracer.scene = &scene;
		int groupN = max(min(QThread::idealThreadCount() - 1, 8), 1);
		rayTracer.camera.ar = (float)viewport->width() / viewport->height();
		for(int i = 0; i < groupN; i++)
		{
			auto rs = new RenderSlave(i);
			slaves.push_back(rs);
			QThread* thread = new QThread();
			rs->moveToThread(thread);
			
			connect(this, SIGNAL(nextTraceStep(RayTracer*, int, int, int, int, SampleDebugger*)), 
				rs, SLOT(traceStep(RayTracer*, int, int, int, int, SampleDebugger*)));
			connect(rs, SIGNAL(traceStepFinished(int)), this, SLOT(traceStepFinished(int)));
			
			thread->start();
		}
	}
signals:
	void nextTraceStep(RayTracer*, int, int, int, int, SampleDebugger*);
public slots:
	void traceStepFinished(int group)
	{
		emit nextTraceStep(&rayTracer, group, slaves.size(), viewport->size().width(), viewport->size().height(), &sampleDebugger);
	}
public:
	virtual void run()
	{		
		for(int i = 0; i < slaves.size(); i++)
		{
			emit nextTraceStep(&rayTracer, i, slaves.size(), viewport->size().width(), viewport->size().height(), &sampleDebugger);
		}
		while(true)
		{		
			rayTracer.camera.ar = (float)viewport->width() / viewport->height();
		}
	}
	SampleDebugger sampleDebugger;
	Viewport* viewport;
	vector<RenderSlave*> slaves;
	unique_ptr<StaticScene> static_scene;
	RTScene scene;
	RayTracer rayTracer;
};
*/