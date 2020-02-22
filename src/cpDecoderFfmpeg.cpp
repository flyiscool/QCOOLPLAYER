#include "stdafx.h"
#include <QtWidgets/QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>
#include <QThread>
#include <queue>
#include <time.h>
#include "cpThread.h"

#include <stdio.h>

#include "cpThreadSafeQueue.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
};

#define SIZE_READ_BUFF 32768
#define MAX_FRAME_TO_SLEEP	5
#define MAX_FRAME_IN_LIST_TO_SHOW	30


static FILE* g_fpVedio = NULL;

extern threadsafe_queue<QImage*> gListToShow;

extern threadsafe_queue<UsbBuffPackage*> gListUsbBulkList_Vedio1;
extern threadsafe_queue<UsbBuffPackage*> gListH264ToUDP;

static	CPThreadDecoderFfmpeg* pthDecoderFfmpeg;

//Callback
static int readFileCallBack(void* opaque, uint8_t* buf, int buf_size) 
{
	int true_size = fread(buf, 1, buf_size, g_fpVedio);
	
	if (feof(g_fpVedio)) {
		rewind(g_fpVedio);
	}

	return true_size;
}

//Callback
static int readUsbVedio1ListCallBack(void* opaque, uint8_t* buf, int buf_size)
{
	UsbBuffPackage* pBuff;
	int length = 0;
	int need = buf_size;

	static int cnt = 0;
	static int pktID = 0;
	int surplus = 0;
	
	do {



		while (gListUsbBulkList_Vedio1.empty())
		{
			pthDecoderFfmpeg->msleep(5);

			if (!(pthDecoderFfmpeg->IsRun())) {
				return length;
			}
		}

		gListUsbBulkList_Vedio1.try_front(pBuff);
		
		if (pktID != pBuff->packageID)  // package has been lost£¬so give up
		{
			cnt = 0;
		}

		if (cnt > 0)  // have half package ; pBuff->length must <= buf_size
		{	 
			surplus = pBuff->length - cnt; 
			memcpy(buf + length, &pBuff->data[cnt], surplus);
			length = length + surplus;
			buf_size = buf_size - surplus;
			
			cnt = 0;
			
			gListUsbBulkList_Vedio1.try_pop(pBuff);
			delete pBuff;
		}
		else
		{
			if (pBuff->length > buf_size)
			{
				//qDebug() << "buf_size" << buf_size << "pBuff->length" << pBuff->length << "pBuff->packageID" << pBuff->packageID << endl;
				cnt = buf_size;
				pktID = pBuff->packageID;
				memcpy(buf + length, pBuff->data, buf_size);
				length = buf_size + length;
				buf_size = 0;
				// don't pop ,wait for next 
			}
			else
			{
				cnt = 0;
				memcpy(buf + length, pBuff->data, pBuff->length);
				length = pBuff->length + length;		
				buf_size = buf_size - pBuff->length;

				gListUsbBulkList_Vedio1.try_pop(pBuff);
				delete pBuff;
			}
		}

		
	} while (buf_size > 0);

	return length;
}




void threadCPDecoderFfmpeg_main(CPThreadDecoderFfmpeg* pCPThreadDecoderFfmpeg)
{
	CPThreadDecoderFfmpeg* pCPthThis = pCPThreadDecoderFfmpeg;
	pthDecoderFfmpeg = pCPThreadDecoderFfmpeg;
	// init ffmpeg
	av_register_all();
	avformat_network_init();

	if (pCPthThis->playMode == SkyTxMode)  // sky play files and tx
	{
		// open file
		QString fileName_H264 = pCPthThis->getFileName();
		QByteArray temp = fileName_H264.toLatin1();
		char* pFileName_H264 = temp.data();

		g_fpVedio = fopen(pFileName_H264, "rb+");
		if (!g_fpVedio) {
			qDebug() << "Can't open video file!" << endl;
			return;
		}
	}
	else  // pCPthThis->playMode == GndRxMode;
	{
		; // do nothing;
	}


	AVFormatContext* pFormatCtx = avformat_alloc_context();

	char errorbuf[1024] = { 0 };
	bool openAgainFlag = true;
	AVIOContext* avio;
	unsigned char* aviobuffer;
	AVInputFormat* pAVInputFmt = NULL;

	do {

		if (!(pCPthThis->IsRun())) {

			if (avio) {
				av_freep(&avio->buffer);
				av_freep(&avio);
			}
			avformat_close_input(&pFormatCtx);
			break;
		}

		//Init AVIOContext    read_buffer is a callback function
		aviobuffer = (unsigned char*)av_malloc(SIZE_READ_BUFF);

	
		if (pCPthThis->playMode == SkyTxMode)
		{
			// Set the call back
			avio = avio_alloc_context(aviobuffer, SIZE_READ_BUFF, 0, NULL, &readFileCallBack, NULL, NULL);
		}
		else
		{
			avio = avio_alloc_context(aviobuffer, SIZE_READ_BUFF, 0, NULL, &readUsbVedio1ListCallBack, NULL, NULL);
		}

		// Set buff to be the input for decoder
		pFormatCtx->pb = avio;

		int r = av_probe_input_buffer(pFormatCtx->pb, &pAVInputFmt, "", NULL, 0, 0);
		if (r == 0)	// success and out
		{
			openAgainFlag = false;
		}
		else if (r > 0)
		{
			openAgainFlag = false;
		}
		else
		{
			av_strerror(r, errorbuf, sizeof(errorbuf));
			qDebug() << "av_probe_input_buffer failed : " << r << QString(QLatin1String(errorbuf)) << endl;

			if (r == -1094995529) //  Invalid data found when processing input
			{
				qDebug() << "try1" << endl;
				openAgainFlag = true;
				if (avio) {
					av_freep(&avio->buffer);
					av_freep(&avio);
				}
				pCPthThis->msleep(10);
				continue;
			}
			else if (r == -572662307)
			{
				qDebug() << "try2" << endl;
				openAgainFlag = true;
				if (avio) {
					av_freep(&avio->buffer);
					av_freep(&avio);
				}
				pCPthThis->msleep(10);
				continue;
			}
			else
			{
				qDebug() << "unknow error so return" << endl;
				return;
			}
		}
	} while (openAgainFlag);


	
	int r = avformat_open_input(&pFormatCtx, NULL, pAVInputFmt, NULL);
	//int r = avformat_open_input(&pFormatCtx, NULL, NULL, NULL);
	if(r != 0)
	{
		qDebug() << "Couldn't open input stream." << endl;
		return;
	}

	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		qDebug() << "Couldn't find stream information." << endl;
		return;
	}

	int videoindex = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	}

	if (videoindex == -1) {
		qDebug() << "Didn't find a video stream." << endl;
		return;
	}

	AVCodecContext* pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodecCtx->thread_count = 4;
	pCodecCtx->thread_type = FF_THREAD_SLICE; // Use of FF_THREAD_FRAME will increase decoding delay by one frame per thread¡£
	pCodecCtx->ticks_per_frame = 2;
	pCodecCtx->bits_per_coded_sample = 1024;

	AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

	if (pCodec == NULL) {
		qDebug() << "Codec not found." << endl;
		return;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		qDebug() << "Could not open codec." << endl;
		return;
	}

	AVFrame* pFrame = av_frame_alloc();
	AVFrame* pFrameRGB = av_frame_alloc();

	unsigned char* rgb_buffer = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height, 1));
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, rgb_buffer,
		AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height, 1);


	AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));

	struct SwsContext* img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);


	int got_picture;

	threadsafe_queue<QImage*>* pList;
	pList = &gListToShow;
	
	while (1) {

		

		if (!(pCPthThis->IsRun())) {
			sws_freeContext(img_convert_ctx);

			av_frame_free(&pFrameRGB);
			av_frame_free(&pFrame);
			av_packet_free(&packet);

			avio_context_free(&avio);

			avcodec_close(pCodecCtx);
			avformat_close_input(&pFormatCtx);

			break;
		}



		if (pCPthThis->playMode == SkyTxMode)
		{

			if (pList->size() >= MAX_FRAME_TO_SLEEP)
			{
				pCPthThis->msleep(50);
				continue;
			}
		}

		if (av_read_frame(pFormatCtx, packet) < 0) {
			pCPthThis->msleep(100);
			continue;
		}

		if (packet->stream_index == videoindex) {
			
			int ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0) {
				av_packet_unref(packet);
				continue;
			}

			if (got_picture) {
				sws_scale(img_convert_ctx,
					(const unsigned char* const*)pFrame->data,
					pFrame->linesize, 0, pCodecCtx->height,
					pFrameRGB->data, pFrameRGB->linesize);

				//QImage tmpImg((uchar*)rgb_buffer, pCodecCtx->width, pCodecCtx->height, QImage::Format_RGB32);
				
				QImage* tmpImg = new QImage((uchar*)rgb_buffer, pCodecCtx->width, pCodecCtx->height, QImage::Format_RGB32);

				QImage* tmpPackegGiveUp = NULL;

				int maxFrameInListToShow;
				if (pCPthThis->playFrameRate == 120)
				{
					maxFrameInListToShow = 1;  // just show the last picture;
				}
				else
				{
					maxFrameInListToShow = 3;
				}
				if (pCPthThis->playMode == SkyTxMode)
				{
					int numret = pList->push(tmpImg, tmpPackegGiveUp, MAX_FRAME_IN_LIST_TO_SHOW);
				}
				else
				{
					int numret = pList->push(tmpImg, tmpPackegGiveUp, maxFrameInListToShow);
				}

				if (tmpPackegGiveUp != NULL)
				{
					delete tmpPackegGiveUp;
				}

				av_packet_unref(packet);
			
			}
		}


	}

}
