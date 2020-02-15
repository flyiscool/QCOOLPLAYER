
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
	connect(ui.actionExit_0, SIGNAL(triggered()), this, SLOT(slot_Exit()));	

	connect(ui.actionOpenFile_H264, SIGNAL(triggered()), this, SLOT(slot_LoadFile()));

}

QCoolPlayer::~QCoolPlayer(void)
{
	slot_Exit();
}

void QCoolPlayer::slot_Exit(void)
{
	close();
}

void QCoolPlayer::slot_LoadFile(void)
{
	QString fileName_H264 = QFileDialog::getOpenFileName(this,
		tr("Open file H.264"), ".", tr("Files (*.264 *.h264)"));

	qDebug() << fileName_H264 << endl;

	

}


