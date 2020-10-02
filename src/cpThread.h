#pragma once
#ifndef CPTHREAD_H
#define CPTHREAD_H


#include <QtWidgets/QMainWindow>
#include <QThread>
#include <QMetaType>
#include <QMutex>
#include <qDebug>

#include "cpStructData.h"
#include "lusb0_usb.h"



class CPThread : public QThread
{
	Q_OBJECT
public slots:
	virtual void stopImmediately()
	{
		m_isCanRun = false;
	}

	QMutex* GetMutex() { return &m_lock; }

	bool IsRun()
	{
		return m_isCanRun;
	};

public:
	QMutex m_lock;
	bool m_isCanRun;
};


// The thread for decode the h264.
class CPThreadDecoderFfmpeg : public CPThread
{
	Q_OBJECT
public:
	virtual void run();
	int playFrameRate;
	bool realTimeMode;
signals:
	void signalFrameRateUpdate(int);
};


// The thread monitor the usb device hot-plug
class CPThreadUsbMonitor : public CPThread
{
	Q_OBJECT
public:
	virtual void run();
	void DoemitSingal(bool arrival);
signals:
	void requestInput();
	void sig_USBUpdate(bool arrival);
};


// The thread bulk the usb video
class CPThreadBulkVideo : public CPThread
{
	Q_OBJECT
public:
	virtual void run();

public slots:
	void updateTheHid(bool);
};



#endif
