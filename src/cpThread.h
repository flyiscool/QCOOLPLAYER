#pragma once
#ifndef CPTHREAD_H
#define CPTHREAD_H


#include <QtWidgets/QMainWindow>
#include <QThread>
#include <QMetaType>
#include <QMutex>
#include <qDebug>

#include "cpStructData.h"
#include "libusb.h"



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
	libusb_device** devsList;
	libusb_context* ctx;
	libusb_device_handle* devGround;
	libusb_device_handle* devSky;
signals:
	void signalUsbStatus(UsbStatus);
};


#endif