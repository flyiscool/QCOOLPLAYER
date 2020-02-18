
#include "stdafx.h"

#include "qcoolplayer.h"

#include <QMenuBar>     
#include <QMenu>        
#include <QAction>
#include <QDebug>
#include <QPainter>
#include <QFileDialog>
#include <Qtimer>

#include "cpThreadSafeQueue.h"

#include "cpUsbMonitor.h"

threadsafe_queue<QImage*> gListToShow;

QCoolPlayer::QCoolPlayer(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	
	ui.verticalLayout->addWidget(&vedioWidget);
	
    // Maximized the window.
	showMaximized();

	//ui.verticalLayout->removeWidget(vedioWidget);
	//vedioWidget->setWindowFlags(Qt::Window);
	//vedioWidget->showFullScreen();

	// set the background
	vedioWidget.setFrame(QImage("./picture/LOGO3-red.png"));

	// open the DX 
	this->setAttribute(Qt::WA_PaintOnScreen, true);
	vedioWidget.setAttribute(Qt::WA_PaintOnScreen, true);

	// set the default data
	vedioWidget.frameRateToShow = 30;  // FPS

	// set the time  tol : 1ms
	timerFreshImage.setTimerType(Qt::TimerType::PreciseTimer);

	// set default mode
	modePlayer = SkyTxMode;


	/*  slot */
	// add the action Exit();
	connect(ui.actionExit_0, SIGNAL(triggered()), this, SLOT(slotExit()));	

	// add the action LoadFile();
	connect(ui.actionOpenFile_H264, SIGNAL(triggered()), this, SLOT(slotLoadFile()));

	// add the action SET_FPS
	connect(ui.action24Fps, SIGNAL(triggered()), this, SLOT(slotSelect24FPS()));
	connect(ui.action25Fps, SIGNAL(triggered()), this, SLOT(slotSelect25FPS()));
	connect(ui.action30Fps, SIGNAL(triggered()), this, SLOT(slotSelect30FPS()));
	connect(ui.action50Fps, SIGNAL(triggered()), this, SLOT(slotSelect50FPS()));
	connect(ui.action60Fps, SIGNAL(triggered()), this, SLOT(slotSelect60FPS()));

	// add the action full screen
	connect(ui.actionFullScreen_F11, SIGNAL(triggered()), this, SLOT(slotSetVedioFullScreen()));

	// add the Show fucntion
	connect(&timerFreshImage, SIGNAL(timeout()), this, SLOT(slotShowTheNewImage()));
	
	// add the keyPress on vedioWidget
	connect(&vedioWidget, SIGNAL(signalKeyPress(QKeyEvent*)), this, SLOT(slotSubWidgetKeyPress(QKeyEvent*)));


	// add the stopPlayVedio action
	connect(ui.actionStop, SIGNAL(triggered()), this, SLOT(slotStopPlayVedio()));

	// add the Sky Gnd mode select
	connect(ui.actionSkyMode, SIGNAL(triggered()), this, SLOT(slotSelectSkyTxMode()));
	connect(ui.actionGroundMode, SIGNAL(triggered()), this, SLOT(slotSelectGndRxMode()));

	// add the usbdevice detect
	
	connect(ui.actionConnectGround, SIGNAL(triggered()), this, SLOT(slotStartOrStopUsbMonitor()));

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


void QCoolPlayer::slotLoadFile(void)
{
	//QString fileName_H264 = QFileDialog::getOpenFileName(this,
	//	tr("Open file H.264"), ".", tr("Files (*.264 *.h264)"));

	//QString fileName_H264 = "bigbuckbunny_480x272.h264";
	QString fileName_H264 = "Avatar_1920x1080@30Hz_3Mbps.264";
	
	thDecoderFfmpeg.fileNameH264 = fileName_H264;

	timerFreshImage.start(1000 / vedioWidget.frameRateToShow);

	ui.actionOpenFile_H264->setEnabled(false);
	ui.actionStop->setEnabled(true);

	ui.actionGroundMode->setEnabled(false);
	ui.actionSkyMode->setEnabled(false);

	thDecoderFfmpeg.start();
}

void QCoolPlayer::slotShowTheNewImage(void)
{
	QImage* img = new QImage;
	gListToShow.try_front(img);
	gListToShow.try_pop(img);

	vedioWidget.setFrame(*img);

	QString tmode;
	if (modePlayer == SkyTxMode)
	{
		tmode = "SkyTxMode";
	}
	else
	{
		tmode = "GndRxMode";
	}

	ui.statusBar->showMessage(tmode + "  ""Fps:" + QString::number(vedioWidget.frameRateToShow, 10));

	delete img;
}

void QCoolPlayer::clearFrameRate(void)
{
	ui.action24Fps->setChecked(false);
	ui.action25Fps->setChecked(false);
	ui.action30Fps->setChecked(false);
	ui.action50Fps->setChecked(false);
	ui.action60Fps->setChecked(false);
}



void QCoolPlayer::slotSelect24FPS(void)
{
	clearFrameRate();
	ui.action24Fps->setChecked(true);
	vedioWidget.frameRateToShow = 24;
	timerFreshImage.setInterval(1000 / vedioWidget.frameRateToShow);
}


void QCoolPlayer::slotSelect25FPS(void)
{
	clearFrameRate();
	ui.action25Fps->setChecked(true);
	vedioWidget.frameRateToShow = 25;
	timerFreshImage.setInterval(1000 / vedioWidget.frameRateToShow);
}

void QCoolPlayer::slotSelect30FPS(void)
{
	clearFrameRate();
	ui.action30Fps->setChecked(true);
	vedioWidget.frameRateToShow = 30;
	timerFreshImage.setInterval(1000 / vedioWidget.frameRateToShow);
}

void QCoolPlayer::slotSelect50FPS(void)
{
	clearFrameRate();
	ui.action50Fps->setChecked(true);
	vedioWidget.frameRateToShow = 50;
	timerFreshImage.setInterval(1000 / vedioWidget.frameRateToShow);
}

void QCoolPlayer::slotSelect60FPS(void)
{
	clearFrameRate();
	ui.action60Fps->setChecked(true);
	vedioWidget.frameRateToShow = 60;
	timerFreshImage.setInterval(1000 / vedioWidget.frameRateToShow);
}

void QCoolPlayer::slotSubWidgetKeyPress(QKeyEvent* ev)
{
	keyPressEvent(ev);
}

void QCoolPlayer::slotSetVedioFullScreen(void)
{
	setVedioFullScreen();
}

void QCoolPlayer::slotStopPlayVedio(void)
{
	thDecoderFfmpeg.stopImmediately();
	thDecoderFfmpeg.wait();

	timerFreshImage.stop();

	ui.actionStop->setEnabled(false);
	ui.actionOpenFile_H264->setEnabled(true);
	ui.actionGroundMode->setEnabled(true);
	ui.actionSkyMode->setEnabled(true);

	vedioWidget.setFrame(QImage("./picture/LOGO3-red.png"));

}

void QCoolPlayer::slotSelectSkyTxMode(void)
{
	ui.actionGroundMode->setChecked(false);
	ui.actionSkyMode->setChecked(true);
	
	ui.actionOpenFile_H264->setEnabled(true);

	modePlayer = SkyTxMode;
}

void QCoolPlayer::slotSelectGndRxMode(void)
{
	ui.actionGroundMode->setChecked(true);
	ui.actionSkyMode->setChecked(false);

	ui.actionOpenFile_H264->setEnabled(false);

	modePlayer = GndRxMode;
}

void QCoolPlayer::slotStartOrStopUsbMonitor(void)
{

	qDebug() << "test" << ui.actionConnectGround->isChecked() <<endl;
	if (ui.actionConnectGround->isChecked() == true)
	{
		thUsbMonitor.start();
	}
	else
	{
		thUsbMonitor.stopImmediately();
		thUsbMonitor.wait();
	}
	// start the USB device Monitor
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
