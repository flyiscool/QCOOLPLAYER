
#include "stdafx.h"

#include "qcoolplayer.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"

#include "SDL.h"

#include <QMenuBar>     
#include <QMenu>        
#include <QAction>
#include <QDebug>

QCoolPlayer::QCoolPlayer(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	
    // Maximized the window.
    showMaximized();
	
	//slot  
	// add the action Exit();
	connect(ui.actionExit_0, SIGNAL(triggered()), this, SLOT(slotExit()));	

	// add the action LoadFile();
	connect(ui.actionOpenFile_H264, SIGNAL(triggered()), this, SLOT(slotLoadFile()));

}

QCoolPlayer::~QCoolPlayer(void)
{
	slotExit();
}

void QCoolPlayer::slotExit(void)
{
	thDecoderFfmpeg.stopImmediately();
	thDecoderFfmpeg.wait();
	close();
}

void QCoolPlayer::slotLoadFile(void)
{
	//QString fileName_H264 = QFileDialog::getOpenFileName(this,
	//	tr("Open file H.264"), ".", tr("Files (*.264 *.h264)"));

	QString fileName_H264 = "Avatar_1920x1080@30Hz_3Mbps.264";
	
	thDecoderFfmpeg.fileNameH264 = fileName_H264;

	thDecoderFfmpeg.start();

}


