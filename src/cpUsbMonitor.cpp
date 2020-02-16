#include "stdafx.h"
#include <QtWidgets/QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>
#include <QThread>
#include <queue>
#include <time.h>
#include "cpThread.h"

//extern "C"
//{
//
//
//
//};


void threadCPUsbMonitor_main(CPThreadUsbMonitor* pCPThreadUsbMonitor)
{
	
	qDebug() << "test the Monitor" << endl;

	while (1){
		if (!(pCPThreadUsbMonitor->IsRun()))
		{
			break;
		}	
	}

}