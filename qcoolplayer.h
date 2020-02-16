#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_qcoolplayer.h"
#include "cpThread.h"
#include "cpRenderArea.h"

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
	CPThreadSdl2Show		thSdl2Show;
	CPThreadUsbMonitor		thUsbMonitor;
	CPThreadUsbVedio1		thUsbVedio1;
	RenderArea*				vedioWidget;
	void clearFrameRate(void);
	
public slots:
	void slotExit(void);
	void slotLoadFile(void);
	void slotShowTheNewImage(QImage img);
	void slotSelect24FPS(void);
	void slotSelect25FPS(void);
	void slotSelect30FPS(void);
	void slotSelect50FPS(void);
	void slotSelect60FPS(void);
	void slotSubWidgetKeyPress(QKeyEvent* ev);
	void slotSetVedioFullScreen(void);

protected:

	virtual void keyPressEvent(QKeyEvent* ev);

};
