/********************************************************************************
** Form generated from reading UI file 'lightway.ui'
**
** Created: Mon Apr 30 19:06:46 2012
**      by: Qt User Interface Compiler version 4.8.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LIGHTWAY_H
#define UI_LIGHTWAY_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QSplitter>
#include <QtGui/QStatusBar>
#include <QtGui/QTableView>
#include <QtGui/QToolBar>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_LightwayClass
{
public:
    QWidget *centralWidget;
    QHBoxLayout *horizontalLayout;
    QSplitter *splitter;
    QWidget *widget;
    QHBoxLayout *horizontalLayout_2;
    QTableView *tableView;
    QMenuBar *menuBar;
    QMenu *menuFile;
    QMenu *menuRender;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *LightwayClass)
    {
        if (LightwayClass->objectName().isEmpty())
            LightwayClass->setObjectName(QString::fromUtf8("LightwayClass"));
        LightwayClass->resize(845, 628);
        centralWidget = new QWidget(LightwayClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        horizontalLayout = new QHBoxLayout(centralWidget);
        horizontalLayout->setSpacing(6);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        splitter = new QSplitter(centralWidget);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        splitter->setOrientation(Qt::Horizontal);
        widget = new QWidget(splitter);
        widget->setObjectName(QString::fromUtf8("widget"));
        horizontalLayout_2 = new QHBoxLayout(widget);
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        splitter->addWidget(widget);
        tableView = new QTableView(splitter);
        tableView->setObjectName(QString::fromUtf8("tableView"));
        splitter->addWidget(tableView);

        horizontalLayout->addWidget(splitter);

        LightwayClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(LightwayClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 845, 21));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        menuRender = new QMenu(menuBar);
        menuRender->setObjectName(QString::fromUtf8("menuRender"));
        LightwayClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(LightwayClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        LightwayClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(LightwayClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        LightwayClass->setStatusBar(statusBar);

        menuBar->addAction(menuFile->menuAction());
        menuBar->addAction(menuRender->menuAction());

        retranslateUi(LightwayClass);

        QMetaObject::connectSlotsByName(LightwayClass);
    } // setupUi

    void retranslateUi(QMainWindow *LightwayClass)
    {
        LightwayClass->setWindowTitle(QApplication::translate("LightwayClass", "Lightway", 0, QApplication::UnicodeUTF8));
        menuFile->setTitle(QApplication::translate("LightwayClass", "Scene", 0, QApplication::UnicodeUTF8));
        menuRender->setTitle(QApplication::translate("LightwayClass", "Render", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class LightwayClass: public Ui_LightwayClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LIGHTWAY_H
