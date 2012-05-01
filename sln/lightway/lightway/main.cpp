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


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	
	Lightway w;
	w.show();

	
	return a.exec();
}
