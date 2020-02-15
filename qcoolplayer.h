#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_qcoolplayer.h"

class QCoolPlayer : public QMainWindow
{
	Q_OBJECT

public:
	QCoolPlayer(QWidget *parent = Q_NULLPTR);
	~QCoolPlayer(void);

private:
	Ui::QCoolPlayerClass ui;
	

public slots:
	void slot_Exit(void);
	void slot_LoadFile(void);
};
