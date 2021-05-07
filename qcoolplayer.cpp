
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
#include <windows.h>

//WINBASEAPI VOID WINAPI GetSystemTime(LPSYSTEMTIME lpSystemTime);

threadsafe_queue<ImgPackage*> gListToShow;
threadsafe_queue<UsbBuffPackage*> gListH264ToUDP;
threadsafe_queue<UsbBuffPackage*> gListUsbBulkList_Vedio1;

bool	flagTakeVideo;

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
	qRegisterMetaType<UsbStatus>("UsbStatus");
	connect(&thUsbMonitor, SIGNAL(signalUsbStatus(UsbStatus)), this, SLOT(slotShowUsbStatus(UsbStatus)));


	// add the usbdevice detect
	connect(ui.actiontake_picture, SIGNAL(triggered()), this, SLOT(slotTakePicture()));

	connect(ui.actionrecord_video, SIGNAL(triggered()), this, SLOT(slotTakeVideo()));

	// add the famre rate update
	//connect(&thUsbMonitor, SIGNAL(signalFrameRateUpdate(int)), this, SLOT(slotUpdateFrameRate(int)));

	thDecoderFfmpeg.realTimeMode = false;
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
	thUsbMonitor.stopImmediately();
	
	thDecoderFfmpeg.wait();
	thUsbMonitor.wait();
	close();
}


void QCoolPlayer::slotShowTheNewImage(void)
{

	QDateTime current_date_time = QDateTime::currentDateTime();
	QString current_date = current_date_time.toString("hh:mm:ss.zzz");
	
	ui.statusBar->showMessage("Fps : 30   Vedio1List : " + QString::number(gListToShow.size(), 10)
		+ "    Vedio1UsbList :" + QString::number(gListUsbBulkList_Vedio1.size(), 10) +
		+"   time:" + current_date, 1000);

	ImgPackage* img;
	if (gListToShow.empty())
	{
		return;
	}

	if (gListToShow.size() == MAX_FRAME_IN_LIST_TO_SHOW)
	{
		for (int i = 0; i < MAX_FRAME_IN_LIST_TO_SHOW - 1; i++)
		{
			gListToShow.try_pop(img);

			delete img;
		}
	}

	gListToShow.try_pop(img);


	if (flagTakePic == true)
	{
		flagTakePic = false;

		ui.statusBar->showMessage("Take picture success", 5000);

		QString current_date = current_date_time.toString("yyyyMMddHHmmss");
		QString filename = "./";
		filename.append(current_date);
		filename.append(".png");

		img->img.save(filename);
		
	}
	

	vedioWidget.setFrame(img->img);

	delete img;
	if (gListToShow.size() > 3)
	{
		if (ui.actionRealTime->isChecked() == true)
		{
			timerFreshImage.setInterval(1000 / (thDecoderFfmpeg.playFrameRate *2 ) - 3);
		}
		else
		{
			timerFreshImage.setInterval(1000 / thDecoderFfmpeg.playFrameRate - 7);
		}

	}
	else
	{
		if (ui.actionRealTime->isChecked() == true)
		{
			timerFreshImage.setInterval(1000 / (thDecoderFfmpeg.playFrameRate * 2));
		}
		else
		{
			timerFreshImage.setInterval(1000 / thDecoderFfmpeg.playFrameRate);
		}
	}
	
}

void QCoolPlayer::slotSelectRealtime(void)
{
	if (ui.actionRealTime->isChecked() == true)
	{
		thDecoderFfmpeg.realTimeMode = true;
		timerFreshImage.setInterval(1000/ (thDecoderFfmpeg.playFrameRate *2));
	}
	else
	{
		thDecoderFfmpeg.realTimeMode = false;
		timerFreshImage.setInterval(1000 / (thDecoderFfmpeg.playFrameRate));
	}
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
		timerFreshImage.start(33);

		thDecoderFfmpeg.start();

		thUsbMonitor.start();

		ui.actionStop->setEnabled(true);

		ui.statusBar->showMessage("connecting...",5000);
	}
	else
	{
		thUsbMonitor.stopImmediately();
		thDecoderFfmpeg.stopImmediately();
		
		timerFreshImage.stop();

		thDecoderFfmpeg.wait();
		thUsbMonitor.wait();
		vedioWidget.setFrame(QImage("./picture/LOGO3-red.png"));
	}
	// start the USB device Monitor
}

void QCoolPlayer::slotShowUsbStatus(UsbStatus status)
{

	QString srtStatus;
	switch (status)
	{
		case NoDeviceFind:  srtStatus = "NoDeviceFind"; break;
		case ConnectSuccess: srtStatus = "ConnectSuccess"; break;
		case ErrorInOpen: srtStatus = "ErrorInOpen"; break;
		case ErrorIncommunication: srtStatus = "ErrorIncommunication";  break;

		default : srtStatus = "ErrorIncommunication"; break;
	}
	ui.statusBar->showMessage(srtStatus,5000);
}


void QCoolPlayer::slotTakePicture()
{
	flagTakePic = true;
}

void QCoolPlayer::slotTakeVideo()
{
	if (ui.actionrecord_video->isChecked() == true)
	{
		flagTakeVideo = true;
	}
	else
	{
		flagTakeVideo = false;
	}

}
void QCoolPlayer::slotUpdateFrameRate(int rate)
{
	//timerFreshImage.setInterval(1000/ thDecoderFfmpeg.playFrameRate);
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
