#include "stdafx.h"
#include <QtWidgets/QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>
#include <QThread>
#include <queue>
#include <time.h>

#include "cpThread.h"
#include "libusb.h"
#include "cpThreadSafeQueue.h"

#define VID_COOLFLY		0xC001

#define PID_CP_F1_SKY   0x0001
#define PID_CP_F1_GND   0x00F1

#define IFGROUND	0

#define ENDPOINT_VEDIO_OUT	0x06 
#define ENDPOINT_VEDIO_IN	0x86 

#define MAX_BUFFER_IN_VEDIO1_LIST	30

extern threadsafe_queue<UsbBuffPackage*> gListUsbBulkList_Vedio1;
extern threadsafe_queue<UsbBuffPackage*> gListH264ToUDP;

int gettimeofday(struct timeval* tp, void* tzp)
{

	static const DWORDLONG FILETIME_to_timval_skew = 116444736000000000;
	FILETIME   tfile;
	::GetSystemTimeAsFileTime(&tfile);

	ULARGE_INTEGER _100ns;
	_100ns.LowPart = tfile.dwLowDateTime;
	_100ns.HighPart = tfile.dwHighDateTime;

	_100ns.QuadPart -= FILETIME_to_timval_skew;

	// Convert 100ns units to seconds;
	ULARGE_INTEGER largeint;
	largeint.QuadPart = _100ns.QuadPart / (10000 * 1000);

	// Convert 100ns units to seconds;
	tp->tv_sec = (long)(_100ns.QuadPart / (10000 * 1000));
	// Convert remainder to microseconds;
	tp->tv_usec = (long)((_100ns.QuadPart % (10000 * 1000)) / 10);

	return (0);
}



void threadCPUsbMonitor_main(CPThreadUsbMonitor* pCPThreadUsbMonitor)
{
	CPThreadUsbMonitor* pCPthThis = pCPThreadUsbMonitor;

	pCPthThis->ctx = NULL;
	pCPthThis->devGround = NULL;
	int r = libusb_init(&pCPthThis->ctx); //initialize a library session
	if (r)
	{
		qDebug() << " libusb init libusb Error = " << r << endl;
		return;
	}

	libusb_set_debug(pCPthThis->ctx, 3);  //set verbosity level to 3, as suggested in the documentation


	FILE* fp_h264 = fopen("record.h264","wb");

	UsbBuffPackage* pVedio1Buff;
	threadsafe_queue<UsbBuffPackage*>* tListToDecode = &gListUsbBulkList_Vedio1;
	threadsafe_queue<UsbBuffPackage*>* tListToUDP = &gListH264ToUDP;
	int transferred;
	struct timeval cap_systime;
	int pktID = 0;
	UsbBuffPackage* pkgGiveUp = NULL;
	int cnt_dev2 = libusb_get_device_list(NULL, &pCPthThis->devsList); //get the list of devices
	qDebug() << cnt_dev2 << endl;
	while (1){



		// exit 
		if (!(pCPthThis->IsRun())) {

			if (pCPthThis->devGround != NULL)
			{
				r = libusb_release_interface(pCPthThis->devGround, IFGROUND); //release the claimed interface
				if (r != 0) {
					qDebug() << "Cannot Release Interface" << endl;
				}
			}

			if (pCPthThis->devGround != NULL)
			{
				libusb_close(pCPthThis->devGround);
			}
			
			if (pCPthThis->ctx != NULL)
			{
				//libusb_exit(pCPthThis->ctx); //close the sessio
			}
			pCPthThis->ctx = NULL;
	
			return;
		}

		// fresh the dev Ground
		if (pCPthThis->devGround == NULL)
		{
			int cnt_dev = libusb_get_device_list(NULL, &pCPthThis->devsList); //get the list of devices
			if (cnt_dev < 0){
				qDebug() << "Error in enumerating devices !" << endl;
				continue;
			}

			pCPthThis->devGround = libusb_open_device_with_vid_pid(pCPthThis->ctx, VID_COOLFLY, PID_CP_F1_GND);
			if (pCPthThis->devGround == NULL) {
				qDebug() << "Can't open the device Ground" << endl;

				emit pCPthThis->signalUsbStatus(NoDeviceFind);
				
				libusb_free_device_list(pCPthThis->devsList, 1);
				pCPthThis->msleep(1000);
				continue;
			}
			else
			{	
				r = libusb_claim_interface(pCPthThis->devGround, IFGROUND); //claim interface 0 (the first) of device (mine had jsut 1)
				if (r < 0) {
					qDebug() << "Cannot Claim Interface" << endl;
					libusb_free_device_list(pCPthThis->devsList, 1);
					pCPthThis->devGround = NULL;
					continue;
				}
				emit pCPthThis->signalUsbStatus(ConnectSuccess);
			}
		}
		
		pVedio1Buff = new UsbBuffPackage;
		transferred = 0;

		r = libusb_interrupt_transfer(pCPthThis->devGround, ENDPOINT_VEDIO_IN, pVedio1Buff->data, MAX_USB_BULK_SIZE, &transferred, 1000);
		switch (r)
		{
		
		case 0:	
			// success
			pVedio1Buff->length = transferred;
			gettimeofday(&cap_systime, NULL);
			pVedio1Buff->timeStamp = cap_systime.tv_sec + 0.000001 * cap_systime.tv_usec;
			pVedio1Buff->packageID = pktID++;

			if (0 == tListToDecode->push(pVedio1Buff, pkgGiveUp, MAX_BUFFER_IN_VEDIO1_LIST))
			{
				qDebug() << "UsbBuffList Vedio1 is Full,Give Up the oldest package" << endl;
				delete pkgGiveUp;
			}

			if (0 == tListToUDP->push(pVedio1Buff, pkgGiveUp, MAX_BUFFER_IN_VEDIO1_LIST))
			{
				//qDebug() << "UsbBuffList Vedio1 is Full,Give Up the oldest package" << endl;
				delete pkgGiveUp;
			}
			


			break;

		case LIBUSB_ERROR_TIMEOUT:	// timeout also need to check the transferred;
			qDebug() << "LIBUSB_ERROR_TIMEOUT" << endl;
			if (transferred > 0)
			{
				pVedio1Buff->length = transferred;
				gettimeofday(&cap_systime, NULL);
				pVedio1Buff->timeStamp = cap_systime.tv_sec + 0.000001 * cap_systime.tv_usec;
				pVedio1Buff->packageID = pktID++;
				if (0 == tListToDecode->push(pVedio1Buff, pkgGiveUp, MAX_BUFFER_IN_VEDIO1_LIST))
				{
					qDebug() << "UsbBuffList Vedio1 is Full, Give Up the oldest package2" << endl;
					delete pkgGiveUp;
				}
				if (0 == tListToUDP->push(pVedio1Buff, pkgGiveUp, MAX_BUFFER_IN_VEDIO1_LIST))
				{
					//qDebug() << "UsbBuffList Vedio1 is Full,Give Up the oldest package" << endl;
					delete pkgGiveUp;
				}
			}
			else
			{
				qDebug() << "vedio1 bulk timeout,There no Data in a second." << endl;
				pCPthThis->msleep(500);
			}
			break;
		case LIBUSB_ERROR_PIPE:	// the endpoint halted,so  retry to open the interface.
			qDebug() << "vedio1 endpoint halted. LIBUSB_ERROR_PIPE" << endl;
			pCPthThis->devGround = NULL;
			pCPthThis->msleep(500);
			break;
		case LIBUSB_ERROR_OVERFLOW:	 // give up the data because it's maybe lost. need fix.
			qDebug() << "vedio1 Buff is to small. LIBUSB_ERROR_OVERFLOW" << endl;
			break;

		case LIBUSB_ERROR_NO_DEVICE: // maybe lost the device.
			qDebug() << "vedio1 device lost. LIBUSB_ERROR_NO_DEVICE" << endl;
			pVedio1Buff->length = NULL;
			break;
		case LIBUSB_ERROR_BUSY:			
			qDebug() << "vedio1 LIBUSB_ERROR_BUSY" << endl;
			pCPthThis->msleep(500);
			break;
		default:
			qDebug() << "vedio error unknow : "<< r << endl;
			break;
		}

	}

}



