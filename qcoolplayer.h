#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_qcoolplayer.h"

class QCoolPlayer : public QMainWindow
{
	Q_OBJECT

public:
	QCoolPlayer(QWidget *parent = Q_NULLPTR);

private:
	Ui::QCoolPlayerClass ui;
};
