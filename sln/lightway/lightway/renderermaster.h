#pragma once

#include "renderslave.h"
#include "viewport.h"
#include "asset.h"
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
		//static_scene->init_tweaks();
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
			
			connect(this, SIGNAL(nextTraceStep(RayTracer*, int, int, int, int)), 
				rs, SLOT(traceStep(RayTracer*, int, int, int, int)));
			connect(rs, SIGNAL(traceStepFinished(int)), this, SLOT(traceStepFinished(int)));
			
			thread->start();
		}
	}
signals:
	void nextTraceStep(RayTracer*, int, int, int, int);
public slots:
	void traceStepFinished(int group)
	{
		emit nextTraceStep(&rayTracer, group, slaves.size(), viewport->size().width(), viewport->size().height());
	}
public:
	virtual void run()
	{		
		for(int i = 0; i < slaves.size(); i++)
		{
			emit nextTraceStep(&rayTracer, i, slaves.size(), viewport->size().width(), viewport->size().height());
		}
		while(true)
		{		
			rayTracer.camera.ar = (float)viewport->width() / viewport->height();
		}
	}
	DebugDraw dd;
	Viewport* viewport;
	vector<RenderSlave*> slaves;
	unique_ptr<StaticScene> static_scene;
	RTScene scene;
	RayTracer rayTracer;
};