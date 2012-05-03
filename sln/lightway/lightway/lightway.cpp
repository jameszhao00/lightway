#include "stdafx.h"
#include "lightway.h"

#include "Viewport.h"
#include "RenderCore.h"
#include "SampleHistoryViewer.h"
#include "asset.h"

/*
struct mat
{
	union {
		FresnelBlendBrdf fresnel;
		LambertBrdf lambert;
	};
};
*/
Lightway::Lightway(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{


	ui.setupUi(this);
	
	viewport = new Viewport(this);
	setPreviewWidget(viewport);
	historyViewer = new SampleHistoryViewer(this);
	setSampleViewWidget(historyViewer);
	renderCore_ = unique_ptr<RenderCore>(new RenderCore());
	viewport->renderCore = renderCore_.get();
	scene = unique_ptr<RTScene>(new RTScene());
	//scene->scene = load_scene("../../../assets/validation/spec_validation1.obj", float3(0, 0, 0), 1);
	scene->scene = load_scene("../../../assets/validation/caustics_validation1.obj", float3(0, 0, 0), 1);
	//scene->scene = load_scene("../../../assets/bunny.obj", float3(0, 0, 0), 5);
	scene->make_accl();
	renderCore_->scene = scene.get();
	historyViewer->syncTo(&renderCore_->sampleDebugger().shr);


	connect(ui.actionRecord_10, SIGNAL(triggered()), this, SLOT(record10()));
	connect(ui.actionRecord_50, SIGNAL(triggered()), this, SLOT(record50()));
	connect(&renderCore_->sampleDebugger().shr, SIGNAL(finishedRecording()), this, SLOT(recordingFinished()));
}

void Lightway::closeEvent(QCloseEvent *event)
{
	renderCore_->stopWorkThreads();
}
void Lightway::recordingFinished()
{		
	ui.actionRecord_10->setEnabled(true);
	ui.actionRecord_50->setEnabled(true);
}
void Lightway::record10()
{
	ui.actionRecord_10->setEnabled(false);
	ui.actionRecord_50->setEnabled(false);
	renderCore_->sampleDebugger().shr.beginRecording(int2(viewport->selectedPoint.x(), viewport->selectedPoint.y()), 10);
}
void Lightway::record50()
{		
	ui.actionRecord_10->setEnabled(false);
	ui.actionRecord_50->setEnabled(false);
	renderCore_->sampleDebugger().shr.beginRecording(int2(viewport->selectedPoint.x(), viewport->selectedPoint.y()), 50);
}
Lightway::~Lightway()
{

}


void Lightway::startRendering()
{
	renderCore_->startWorkThreads();
}
void Lightway::stopRendering()
{
	renderCore_->stopWorkThreads();
}