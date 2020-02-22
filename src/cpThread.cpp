
#include "stdafx.h"
#include "cpThread.h"

#include "cpDecoderFfmpeg.h"
#include "cpUsbMonitor.h"
#include "cpEncoderToUDP.h"



void CPThreadDecoderFfmpeg::run()
{
	m_isCanRun = true;
	threadCPDecoderFfmpeg_main(this);
}

void CPThreadEncoderToUDP::run()
{
	m_isCanRun = true;
	threadCPEncoderToUDP_main(this);
}

void CPThreadUsbMonitor::run()
{
	m_isCanRun = true;
	threadCPUsbMonitor_main(this);
}

void CPThreadUsbVedio1::run()
{
	m_isCanRun = true;
	//threadCPUsbMonitor_main(this);
}
