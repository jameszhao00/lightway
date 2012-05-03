#ifndef LIGHTWAY_H
#define LIGHTWAY_H

#include <QtGui/QMainWindow>
#include "ui_lightway.h"
#include <memory>
using namespace std;

class Viewport;
class SampleHistoryViewer;
class RenderCore;
class RTScene;
class Lightway : public QMainWindow
{
	Q_OBJECT

public:
	Lightway(QWidget *parent = 0, Qt::WFlags flags = 0);
	~Lightway();
	void setSampleViewWidget(QWidget* widget)
	{
		ui.widget_2->layout()->addWidget(widget);
	}
	void setPreviewWidget(QWidget* widget)
	{
		ui.widget->layout()->addWidget(widget);
		widget->setMaximumSize(600, 600);
		widget->setMinimumSize(600, 600);
	}
	void startRendering();
	void stopRendering();
public slots:
	
	void recordingFinished();
	void record10();
	void record50();
protected:	
	void closeEvent(QCloseEvent *event);
private:
	unique_ptr<RTScene> scene;

	Ui::LightwayClass ui;
	Viewport* viewport;
	SampleHistoryViewer* historyViewer;
	unique_ptr<RenderCore> renderCore_;
};

#endif // LIGHTWAY_H
