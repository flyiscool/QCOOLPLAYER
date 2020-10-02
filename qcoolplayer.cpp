
#include "stdafx.h"

#include "qcoolplayer.h"

#include <QMenuBar>     
#include <QMenu>        
#include <QAction>
#include <QDebug>
#include <QPainter>
#include <QFileDialog>
#include <Qtimer>
#include <QMetaType>
#include "cpThreadSafeQueue.h"

#include "cpUsbMonitor.h"
#include "cpBulkVideo.h"

threadsafe_queue<QImage*> gListToShow;
threadsafe_queue<UsbBuffPackage*> gListH264ToUDP;
threadsafe_queue<UsbBuffPackage*> gListUsbBulkList_Vedio1;

CPThreadUsbMonitor*		g_thUsbMonitor;

QCoolPlayer::QCoolPlayer(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	
	ui.verticalLayout->addWidget(&vedioWidget);
	
    // Maximized the window.
	//showMaximized();

	//ui.verticalLayout->removeWidget(vedioWidget);
	//vedioWidget->setWindowFlags(Qt::Window);
	//vedioWidget->showFullScreen();

	// set the background
	vedioWidget.setFrame(QImage("./picture/LOGO3-red.png"));

	// open the DX 
	this->setAttribute(Qt::WA_PaintOnScreen, true);
	vedioWidget.setAttribute(Qt::WA_PaintOnScreen, true);

	// set the time  tol : 1ms
	timerFreshImage.setTimerType(Qt::TimerType::PreciseTimer);

	g_thUsbMonitor = &thUsbMonitor;

	/*  slot */
	// add the action Exit();
	connect(ui.actionExit_0, SIGNAL(triggered()), this, SLOT(slotExit()));	

	// add the action LoadFile();
	connect(ui.actionOpenFile_H264, SIGNAL(triggered()), this, SLOT(slotLoadFile()));

	// add the action SET_FPS
	connect(ui.actionRealTime, SIGNAL(triggered()), this, SLOT(slotSelectRealtime()));


	// add the action full screen
	connect(ui.actionFullScreen_F11, SIGNAL(triggered()), this, SLOT(slotSetVedioFullScreen()));

	// add the Show fucntion
	connect(&timerFreshImage, SIGNAL(timeout()), this, SLOT(slotShowTheNewImage()));
	
	// add the keyPress on vedioWidget
	connect(&vedioWidget, SIGNAL(signalKeyPress(QKeyEvent*)), this, SLOT(slotSubWidgetKeyPress(QKeyEvent*)));

	// add the usbdevice detect
	connect(ui.actionConnect, SIGNAL(triggered()), this, SLOT(slotStartOrStopUsbMonitor()));

	// add the UsbStatus show in statusBar
	connect(&thUsbMonitor, SIGNAL(sig_USBUpdate(bool)), this, SLOT(slotShowUsbStatus(bool)));
	connect(&thUsbMonitor, SIGNAL(sig_USBUpdate(bool)), &thBulkVideo, SLOT(updateTheHid(bool)));



	// add the famre rate update
	connect(&thUsbMonitor, SIGNAL(signalFrameRateUpdate(int)), this, SLOT(slotUpdateFrameRate(int)));

	thDecoderFfmpeg.realTimeMode = true;
	thDecoderFfmpeg.playFrameRate = 30;  // default

	//thEncoderToUDP.start();
}

QCoolPlayer::~QCoolPlayer(void)
{
	slotExit();
}

void QCoolPlayer::slotExit(void)
{
	thDecoderFfmpeg.stopImmediately();
	thBulkVideo.stopImmediately();
	thUsbMonitor.stopImmediately();
	
	thDecoderFfmpeg.wait();
	thBulkVideo.wait();
	thUsbMonitor.wait();

	
	close();
}


void QCoolPlayer::slotShowTheNewImage(void)
{
	QImage* img = new QImage;
	if (gListToShow.empty())
	{
		return;
	}

	gListToShow.try_pop(img);

	vedioWidget.setFrame(*img);

	gListToShow.size();
	ui.statusBar->showMessage("Fps : " + QString::number(thDecoderFfmpeg.playFrameRate, 10)
		+ "    Vedio1List : " + QString::number(gListToShow.size(), 10)
		+ "    Vedio1UsbList :" + QString::number(gListUsbBulkList_Vedio1.size(), 10), 1000);

	delete img;
}

void QCoolPlayer::slotSelectRealtime(void)
{
	if (ui.actionRealTime->isChecked() == true)
	{
		thDecoderFfmpeg.realTimeMode = true;
	}
	else
	{
		thDecoderFfmpeg.realTimeMode = false;
	}
	timerFreshImage.setInterval(1000 / (thDecoderFfmpeg.playFrameRate * 2));
}

void QCoolPlayer::slotSubWidgetKeyPress(QKeyEvent* ev)
{
	keyPressEvent(ev);
}

void QCoolPlayer::slotSetVedioFullScreen(void)
{
	setVedioFullScreen();
}

void QCoolPlayer::slotStartOrStopUsbMonitor(void)
{
	if (ui.actionConnect->isChecked() == true)
	{
		timerFreshImage.start(1000 / thDecoderFfmpeg.playFrameRate);

		thDecoderFfmpeg.start();
		thBulkVideo.start();
		thUsbMonitor.start();

		ui.actionStop->setEnabled(true);

		ui.statusBar->showMessage("connecting...",5000);
	}
	else
	{
		thUsbMonitor.stopImmediately();
		thBulkVideo.stopImmediately();
		thDecoderFfmpeg.stopImmediately();
		
		timerFreshImage.stop();

		thDecoderFfmpeg.wait();
		thBulkVideo.wait();
		thUsbMonitor.wait();
		vedioWidget.setFrame(QImage("./picture/LOGO3-red.png"));
	}
	// start the USB device Monitor
}

void QCoolPlayer::slotShowUsbStatus(bool flag)
{

	QString srtStatus;
	switch (flag)
	{
		case false:  srtStatus = "NoDeviceFind"; break;
		case true: srtStatus = "ConnectSuccess"; break;
		default : srtStatus = "ErrorIncommunication"; break;
	}
	ui.statusBar->showMessage(srtStatus,5000);
}

void QCoolPlayer::slotUpdateFrameRate(int rate)
{
	timerFreshImage.setInterval(1000 / thDecoderFfmpeg.playFrameRate);
}


void QCoolPlayer::setVedioFullScreen(void)
{
	ui.verticalLayout->removeWidget(&vedioWidget);
	vedioWidget.setWindowFlags(Qt::Window);
	vedioWidget.showFullScreen();
}

void QCoolPlayer::setVedioNormalScreen(void)
{
	ui.verticalLayout->addWidget(&vedioWidget);
	vedioWidget.setWindowFlags(Qt::SubWindow);
	vedioWidget.showNormal();
}

void QCoolPlayer::keyPressEvent(QKeyEvent* ev)
{

	if (ev->key() == Qt::Key_F11)
	{
		setVedioFullScreen();
		return;
	}
	else if (ev->key() == Qt::Key_Escape)
	{
		setVedioNormalScreen();
	}

	QWidget::keyPressEvent(ev);
}
