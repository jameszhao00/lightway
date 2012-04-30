#ifndef LIGHTWAY_H
#define LIGHTWAY_H

#include <QtGui/QMainWindow>
#include "ui_lightway.h"

class Lightway : public QMainWindow
{
	Q_OBJECT

public:
	Lightway(QWidget *parent = 0, Qt::WFlags flags = 0);
	~Lightway();
	void setPreviewWidget(QWidget* widget)
	{
		ui.widget->layout()->addWidget(widget);
		widget->setMaximumSize(300, 300);
	}
private:
	Ui::LightwayClass ui;
};

#endif // LIGHTWAY_H
