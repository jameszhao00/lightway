#include "stdafx.h"
#include "lightway.h"
#include <QtGui/QApplication>
#include <QThread>
#include <QtOpenGL>
#include <cstdio>
#include <iostream>
#include <list>
#include "test.h"
int main(int argc, char *argv[])
{
	//testUniformGridAggregate();
	QApplication a(argc, argv);
	
	Lightway w;
	w.show();
	w.startRendering();
	
	return a.exec();
}
