
#include "stdafx.h"

#include "qcoolplayer.h"

#include <QMenuBar>     
#include <QMenu>        
#include <QAction>
#include <QDebug>
#include <QPainter>
#include <QFileDialog>



QCoolPlayer::QCoolPlayer(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	
	vedioWidget = new RenderArea(this);
	ui.verticalLayout->addWidget(vedioWidget);
	
    // Maximized the window.
	showMaximized();

	//ui.verticalLayout->removeWidget(vedioWidget);
	//vedioWidget->setWindowFlags(Qt::Window);
	//vedioWidget->showFullScreen();

	// set the background
	vedioWidget->setFrame(QImage("./picture/LOGO3-red.png"));

	// open the DX 
	this->setAttribute(Qt::WA_PaintOnScreen, true);
	vedioWidget->setAttribute(Qt::WA_PaintOnScreen, true);

	// set the default data
	thDecoderFfmpeg.frameRate = 30;  // FPS

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
	connect(&thDecoderFfmpeg, SIGNAL(signalGetOneFrameToShow(QImage)), this, SLOT(slotShowTheNewImage(QImage)));

	// add the keyPress on vedioWidget
	connect(vedioWidget, SIGNAL(signalKeyPress(QKeyEvent*)), this, SLOT(slotSubWidgetKeyPress(QKeyEvent*)));

	/*  fuction */
	// start the USB device Monitor
	thUsbMonitor.start();

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

	thDecoderFfmpeg.start();

}

void QCoolPlayer::slotShowTheNewImage(QImage img)
{
	vedioWidget->setFrame(img);
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
	thDecoderFfmpeg.frameRate = 24;
}


void QCoolPlayer::slotSelect25FPS(void)
{
	clearFrameRate();
	ui.action25Fps->setChecked(true);
	thDecoderFfmpeg.frameRate = 25;
}

void QCoolPlayer::slotSelect30FPS(void)
{
	clearFrameRate();
	ui.action30Fps->setChecked(true);
	thDecoderFfmpeg.frameRate = 30;
}

void QCoolPlayer::slotSelect50FPS(void)
{
	clearFrameRate();
	ui.action50Fps->setChecked(true);
	thDecoderFfmpeg.frameRate = 50;
}

void QCoolPlayer::slotSelect60FPS(void)
{
	clearFrameRate();
	ui.action60Fps->setChecked(true);
	thDecoderFfmpeg.frameRate = 60;
}

void QCoolPlayer::slotSubWidgetKeyPress(QKeyEvent* ev)
{
	keyPressEvent(ev);
}

void QCoolPlayer::slotSetVedioFullScreen(void)
{
	setVedioFullScreen();
}



void QCoolPlayer::setVedioFullScreen(void)
{
	ui.verticalLayout->removeWidget(vedioWidget);
	vedioWidget->setWindowFlags(Qt::Window);
	vedioWidget->showFullScreen();
}

void QCoolPlayer::setVedioNormalScreen(void)
{
	ui.verticalLayout->addWidget(vedioWidget);
	vedioWidget->setWindowFlags(Qt::SubWindow);
	vedioWidget->showNormal();
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
