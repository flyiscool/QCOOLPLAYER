#pragma once

#include "stdafx.h"

#include <QMetaType>

#define MAX_USB_BULK_SIZE  4*1024     // 32Kbyte 
//#define MAX_USB_BULK_SIZE  1024*1024    // 1Mbyte 
//#define MAX_USB_BULK_SIZE  512*1024     // 512Kbyte 

// data strcut
enum RxTxMode {
    SkyTxMode,
    GndRxMode,
};


enum UsbStatus {
    NoDeviceFind,
    ConnectSuccess,
    ErrorInOpen,
    ErrorIncommunication,
};
Q_DECLARE_METATYPE(UsbStatus)



struct UsbBuffPackage
{
    int packageID;
    double timeStamp;
    int length;
    uint8_t data[MAX_USB_BULK_SIZE];
};


struct ImgPackage
{
    int packageID;
    double timeStamp;
    QImage img;
};
