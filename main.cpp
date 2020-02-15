#include "stdafx.h"
#include "qcoolplayer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QCoolPlayer w;
	w.show();
	return a.exec();
}
