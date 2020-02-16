
#include "stdafx.h"
#include "cpThread.h"
#include "cpDecoderFfmpeg.h"
#include "cpUsbMonitor.h"

void CPThreadDecoderFfmpeg::run()
{
	m_isCanRun = true;
	threadCPDecoderFfmpeg_main(this);
}

void CPThreadSdl2Show::run()
{
	m_isCanRun = true;
	//threadCPSdl2Show_main(this);
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
