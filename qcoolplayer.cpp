#include "stdafx.h"
#include "qcoolplayer.h"
#include "libavcodec/avcodec.h"

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

	qDebug() << "test" << endl;
	
	//slot
	connect(ui.actionExit_0, SIGNAL(triggered()), this, SLOT(slot_Exit(void)));
}

void QCoolPlayer::slot_Exit(void)
{
	close();
}