#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_qcoolplayer.h"
#include "cpThread.h"

class QCoolPlayer : public QMainWindow
{
	Q_OBJECT

public:
	QCoolPlayer(QWidget *parent = Q_NULLPTR);
	~QCoolPlayer(void);

private:
	Ui::QCoolPlayerClass ui;
	CPThreadDecoderFfmpeg	thDecoderFfmpeg;
	CPThreadSdl2Show		thSdl2Show;
	CPThreadUsbMonitor		thUsbMonitor;
	CPThreadUsbVedio1		thUsbVedio1;

public slots:
	void slotExit(void);
	void slotLoadFile(void);
};
