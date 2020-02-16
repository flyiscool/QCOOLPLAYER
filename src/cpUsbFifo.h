#ifndef FIFO_H
#define FIFO_H
#include "stdafx.h"

#include <QtCore>
#include <atlstr.h> // CString
#include"qlist.h"
#include <QTextCodec>
#include <tchar.h>
#include <dbt.h>
#include <iostream>
#include <QString>
#include <QMutex>


using namespace std;

#define FIFLODEEPLENGTH 50*1024*1024

class usbfifo
{
public:
		usbfifo(void);
	virtual ~usbfifo(void);
public:
	long int pwrite;
	long int pread;
	bool full;
	bool empty;
	long int lengthsave;
	bool gotoHeader;
	BYTE ram[ 2 * FIFLODEEPLENGTH ];
	void write(BYTE*, int, int&);
	void read(BYTE*, int, LONG&);
	void initialfifo();
public:	signals:
	bool ack();
protected:
	void run();
};

//extern usbfifo *g_fifo,*g_fifo_codeanalysis,*g_fifo_plotDynamic,*g_fifo_swapfrequent,*g_fifo_audio,*g_fifo_factorySetting;

#endif //FIFO_H