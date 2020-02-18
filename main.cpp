#include "stdafx.h"
#include "qcoolplayer.h"
#include <QtWidgets/QApplication>

//  config for the libusb  
#pragma comment(lib, "legacy_stdio_definitions.lib")

#ifdef __cplusplus
FILE iob[] = { *stdin, *stdout, *stderr };
extern "C" {
	FILE* __cdecl _iob(void) { return iob; }
}
#endif

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QCoolPlayer w;
	w.show();
	return a.exec();
}
