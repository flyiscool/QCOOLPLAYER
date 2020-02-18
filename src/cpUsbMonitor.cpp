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


#define VID_COOLFLY		0xC001

#define PID_CP_F1_SKY   0x0001
#define PID_CP_F1_GND   0x00F1

#define IFGROUND	0

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

			//Only avaiable on linux
			//libusb_attach_kernel_driver(pCPthThis->devGround, 0);
			if (pCPthThis->devGround != NULL)
			{
				libusb_close(pCPthThis->devGround);
			}
			
			if (pCPthThis->ctx != NULL)
			{
				//libusb_exit(pCPthThis->ctx); //close the sessio
			}
			pCPthThis->ctx = NULL;
			
			qDebug() << "end" << endl;

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
				libusb_free_device_list(pCPthThis->devsList, 1);
				pCPthThis->msleep(1000);
				continue;
			}
			else
			{	
				// Only avaiable on linux
				//if (libusb_kernel_driver_active(pCPthThis->devGround, 0) == 1) { //find out if kernel driver is attached
				//	qDebug() << "Kernel Driver Active" << endl;
				//	if (libusb_detach_kernel_driver(pCPthThis->devGround, 0) == 0) { //detach it
				//		qDebug() << "Kernel Driver Detached!" << endl;
				//	}
				//}

				r = libusb_claim_interface(pCPthThis->devGround, IFGROUND); //claim interface 0 (the first) of device (mine had jsut 1)
				if (r < 0) {
					qDebug() << "Cannot Claim Interface" << endl;
					libusb_free_device_list(pCPthThis->devsList, 1);
					pCPthThis->devGround = NULL;
					continue;
				}
			}
		}
		


	}

}



