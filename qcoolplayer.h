#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_qcoolplayer.h"
#include "cpThread.h"
#include "cpRenderArea.h"

#include "cpStructData.h"



class QCoolPlayer : public QMainWindow
{
	Q_OBJECT

public:
	QCoolPlayer(QWidget *parent = Q_NULLPTR);
	~QCoolPlayer(void);
	void setVedioFullScreen(void);
	void setVedioNormalScreen(void);

private:
	Ui::QCoolPlayerClass ui;
	CPThreadDecoderFfmpeg	thDecoderFfmpeg;
	CPThreadUsbMonitor		thUsbMonitor;
	RenderArea				vedioWidget;
	QTimer					timerFreshImage;
public slots:
	void slotExit(void);
	void slotShowTheNewImage(void);
	void slotSelectRealtime(void);
	void slotSubWidgetKeyPress(QKeyEvent* ev);
	void slotSetVedioFullScreen(void);
	void slotStartOrStopUsbMonitor(void);
	void slotShowUsbStatus(UsbStatus status);
	void slotUpdateFrameRate(int rate);

protected:

	virtual void keyPressEvent(QKeyEvent* ev);

};
