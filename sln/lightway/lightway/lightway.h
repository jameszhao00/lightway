#ifndef LIGHTWAY_H
#define LIGHTWAY_H

#include <QtGui/QMainWindow>
#include "ui_lightway.h"

class Viewport;
class SampleHistoryViewer;
class RendererMaster;
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
		widget->setMaximumSize(300, 300);
		widget->setMinimumSize(300, 300);
	}
public slots:
	
	void recordingFinished();
	void record10();
	void record50();
private:
	Ui::LightwayClass ui;
	Viewport* viewport;
	SampleHistoryViewer* historyViewer;
	RendererMaster* renderMaster;
};

#endif // LIGHTWAY_H
