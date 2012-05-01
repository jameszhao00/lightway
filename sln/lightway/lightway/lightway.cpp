#include "stdafx.h"
#include "lightway.h"

#include "Viewport.h"
#include "renderermaster.h"
#include "SampleHistoryViewer.h"

Lightway::Lightway(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);
	
	viewport = new Viewport();
	setPreviewWidget(viewport);
	historyViewer = new SampleHistoryViewer();
	setSampleViewWidget(historyViewer);
	
	renderMaster = new RendererMaster(viewport);
	
	renderMaster->loadScene();
	renderMaster->init();
	renderMaster->start();
	historyViewer->syncTo(&renderMaster->sampleDebugger.shr);


	connect(ui.actionRecord_10, SIGNAL(triggered()), this, SLOT(record10()));
	connect(ui.actionRecord_50, SIGNAL(triggered()), this, SLOT(record50()));
	connect(&renderMaster->sampleDebugger.shr, SIGNAL(finishedRecording()), this, SLOT(recordingFinished()));
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
	renderMaster->sampleDebugger.shr.beginRecording(int2(viewport->selectedPoint.x(), viewport->selectedPoint.y()), 10);
}
void Lightway::record50()
{		
	ui.actionRecord_10->setEnabled(false);
	ui.actionRecord_50->setEnabled(false);
	renderMaster->sampleDebugger.shr.beginRecording(int2(viewport->selectedPoint.x(), viewport->selectedPoint.y()), 50);
}
Lightway::~Lightway()
{

}
