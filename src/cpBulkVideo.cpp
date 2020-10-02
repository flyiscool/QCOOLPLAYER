#include "stdafx.h"
#include <QtWidgets/QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>
#include <QThread>
#include <queue>
#include <time.h>

#include "cpThread.h"
#include "cpThreadSafeQueue.h"
#include "cpStructData.h"
#include "cpBulkVideo.h"

#include "lusb0_usb.h"
#include <Guiddef.h>
#include <list>
#include <tchar.h>
#include <stdio.h>
#include <dbt.h>
#include <atlstr.h> // CString

#define BUFSIZE 1024*1024

#define MY_VID 0xAAAA
#define MY_PID 0xAA97
#define MY_CONFIG 1
#define MY_INTF 0

#define EP_IN4 0x82
#define EP_IN3 0x84
#define EP_IN2 0x86
#define EP_IN1 0x88
#define EP_IN5 0x85
#define EP_OUT1 0x02
#define EP_OUT2 0x01
#define EP_OUT3 0x06
#define BUF_SIZE_HID 512

#define MAX_BUFFER_IN_VEDIO1_LIST	30

extern threadsafe_queue<UsbBuffPackage*> gListUsbBulkList_Vedio1;

usb_dev_handle* COMMONDEV = NULL;
usb_dev_handle* VIDEODEV = NULL;
usb_dev_handle* AUDIODEV = NULL;
usb_dev_handle* CUSTOMERDEV = NULL;

struct usb_bus* bus;
struct usb_device* dev = NULL;
usb_dev_handle* tmpdev[5] = { 0,0,0,0,0 };


extern WPARAM onwParam;

int transfer_bulk_async(usb_dev_handle* dev, int ep, char* bytes, int size, int timeout)
{

	if (dev == NULL)
	{
		return -1;
	}

	void* async_context = NULL;
	int ret;

	// Setup the async transfer.  This only needs to be done once
	// for multiple submit/reaps. (more below)
	//


	if (ep == EP_OUT2)
	{
		ret = usb_interrupt_setup_async(dev, &async_context, ep);
	}
	else
	{
		ret = usb_bulk_setup_async(dev, &async_context, ep);
	}


	if (ret < 0)
	{
		printf("error usb_bulk_setup_async:\n%s\n", usb_strerror());
		goto Done;
	}

	// Submit this transfer.  This function returns immediately and the
	// transfer is on it's way to the device.
	//
	ret = usb_submit_async(async_context, bytes, size);
	if (ret < 0)
	{
		usb_free_async(&async_context);
		goto Done;
	}

	// Wait for the transfer to complete.  If it doesn't complete in the
	// specified time it is cancelled.  see also usb_reap_async_nocancel().
	//

	ret = usb_reap_async(async_context, timeout);

	// Free the context.
	usb_free_async(&async_context);

Done:
	return ret;
}


void threadCPBulkVideo_main(CPThreadBulkVideo* pCPThreadBulkVideo)
{

	CPThreadBulkVideo* pCPthThis = pCPThreadBulkVideo;

	usb_init();		/* initialize the library */
	pCPthThis->updateTheHid(true);

	UsbBuffPackage* pVedio1Buff;
	UsbBuffPackage* pkgGiveUp;
	threadsafe_queue<UsbBuffPackage*>* tListH264 = &gListUsbBulkList_Vedio1;
	unsigned int pktID = 0;


	while (pCPthThis->IsRun())
	{
		if (VIDEODEV != NULL)
		{
			pVedio1Buff = new UsbBuffPackage;

			int res = transfer_bulk_async(VIDEODEV, EP_IN2, (char*)pVedio1Buff->data, MAX_USB_BULK_SIZE, 1000);

			if (res > 0)
			{
				pVedio1Buff->length = res;
				pVedio1Buff->packageID = pktID++;
				if (0 == tListH264->push(pVedio1Buff, pkgGiveUp, 100))
				{
					//printf("H264LIST Full  give up the oldest package !!!\r\n");
					delete pkgGiveUp;
				}
			}
			else
			{
				pCPthThis->msleep(10);
			}
		}
		else
		{
			pCPthThis->msleep(10);
		}
	}
}

extern PDEV_BROADCAST_DEVICEINTERFACE onpDevInf;


void CPThreadBulkVideo::updateTheHid(bool test)
{

	int t = 0;
	int y = t + 1;
	wchar_t szMsg[1000] = { 0 };
	CString szDevId = onpDevInf->dbcc_name + 4;
	if (szDevId == "") return;
	_stprintf(szMsg, L"Connect [%s] failed!", szDevId.GetBuffer());
	CString szTmp;
	szTmp.Format(_T("%s"), szDevId.GetBuffer());

	USES_CONVERSION;
	QString ret = QString::fromUtf16(szTmp);
	//QString ret = QstringFromCstring(szTmp);
	QStringList coords = ret.split('#');
	if (coords.count() != 4) return;
	//HDCOMD iHDCOMD;
	if (coords[0] == "USB")
	{

		iHDCOMD.TYPE = "USB";
		QStringList vid_pid = coords[1].split('&');
		if (vid_pid[0].split('_').count() > 1)
			iHDCOMD.VID = vid_pid[0].split('_')[1];
		iHDCOMD.DEVID = coords[2];
		iHDCOMD.DISP = coords[3];
		if (vid_pid.count() == 2)
		{
			if (vid_pid[1].split('_').count() > 1)
				iHDCOMD.PID = vid_pid[1].split('_')[1];
		}
		if (DBT_DEVICEARRIVAL == onwParam)  iHDCOMD.ADDREM = 1;
		else iHDCOMD.ADDREM = 0;
		Updatedeviceslist();
	}
	else if (coords[0] == "USBSTOR")
	{
		iHDCOMD.TYPE = "USBSTOR";

		QStringList vid_pid = coords[1].split('&');
		if (vid_pid[0].split('_').count() > 1)
			iHDCOMD.VID = vid_pid[0].split('_')[1];
		iHDCOMD.DEVID = coords[2];
		iHDCOMD.DISP = coords[3];
		if (vid_pid.count() == 2) iHDCOMD.PID = vid_pid[1];
		else
		{
			if (vid_pid[1].split('_').count() > 1)
				iHDCOMD.PID = coords[1].split('_')[1];
		}
		if (DBT_DEVICEARRIVAL == onwParam)  iHDCOMD.ADDREM = 1;
		else iHDCOMD.ADDREM = 0;
	}
}




