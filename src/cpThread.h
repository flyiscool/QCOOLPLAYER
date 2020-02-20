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
	QString fileNameH264;
	RxTxMode playMode;
	int playFrameRate;

public slots:
	QString getFileName(void)
	{
		return fileNameH264;
	}

signals:
	void signalGetOneFrameToShow(QImage);
};



// The thread for show the raw image data.
class CPThreadSdl2Show : public CPThread
{
	Q_OBJECT
public:
	virtual void run();
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

class CPThreadUsbVedio1 : public CPThread
{
	Q_OBJECT
public:
	virtual void run();
};


#endif