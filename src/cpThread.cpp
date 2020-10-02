
#include "stdafx.h"
#include "cpThread.h"

#include "cpDecoderFfmpeg.h"
#include "cpUsbMonitor.h"
#include "cpBulkVideo.h"


void CPThreadDecoderFfmpeg::run()
{
	m_isCanRun = true;
	threadCPDecoderFfmpeg_main(this);
}

void CPThreadUsbMonitor::run()
{
	m_isCanRun = true;
	threadCPUsbMonitor_main(this);
}


void CPThreadBulkVideo::run()
{
	m_isCanRun = true;
	threadCPBulkVideo_main(this);
}

