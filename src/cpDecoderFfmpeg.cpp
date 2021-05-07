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
#include "stdio.h"
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



static FILE* g_fpVedio = NULL;

extern threadsafe_queue<ImgPackage*> gListToShow;

extern threadsafe_queue<UsbBuffPackage*> gListUsbBulkList_Vedio1;
extern threadsafe_queue<UsbBuffPackage*> gListH264ToUDP;

static	CPThreadDecoderFfmpeg* pthDecoderFfmpeg;

extern bool	flagTakeVideo;

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
			pktID++;
		}
		else
		{
			if (pBuff->length > buf_size)
			{
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
				pktID++;
			}
		}

	} while (buf_size > 0);

	static bool saveflag = false;
	static QFile* fp = NULL;
	static bool I_flag = false;
	if (flagTakeVideo)
	{
		if (fp == NULL)
		{
			QDateTime current_date_time = QDateTime::currentDateTime();
			QString current_date = current_date_time.toString("yyyyMMddHHmmss");
			QString filename = "./";
			filename.append(current_date);
			filename.append(".h264");
			fp = new QFile(filename);
			fp->open(QIODevice::ReadWrite | QIODevice::Append);
			I_flag = false;
		}
		else
		{
			QDataStream t(fp);
			int j;
			if (I_flag != true)
			{
				for (j = 0; j < need - 4; j++)
				{
					if ((buf[j] == 0x00) && (buf[j + 1] == 0x00) && (buf[j + 2] == 0x00) & (buf[j + 3] == 0x01) & (buf[j + 4] == 0x67))
					{
						I_flag = true;
						t.writeRawData((char*)buf + j, need - j);
						break;
					}
				}
			}
			else
			{
				t.writeRawData((char*)buf, need);
			}
			
			


			//fp->write(buf, buf_size);
			//QByteArray qa2((char *)buf, need);

			//fp->write(qa2);
			fp->flush();


		}


	}
	else
	{
		if (fp != NULL)
		{
			fp->close();
			I_flag = false;
			fp = NULL;
		}
	}




	return length;
}




void threadCPDecoderFfmpeg_main(CPThreadDecoderFfmpeg* pCPThreadDecoderFfmpeg)
{
	CPThreadDecoderFfmpeg* pCPthThis = pCPThreadDecoderFfmpeg;
	pthDecoderFfmpeg = pCPThreadDecoderFfmpeg;
	// init ffmpeg
	av_register_all();
	avformat_network_init();

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
			//avformat_close_input(&pFormatCtx);
			return;
		}

		//Init AVIOContext    read_buffer is a callback function
		aviobuffer = (unsigned char*)av_malloc(SIZE_READ_BUFF);

		avio = avio_alloc_context(aviobuffer, SIZE_READ_BUFF, 0, NULL, &readUsbVedio1ListCallBack, NULL, NULL);
	
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

	threadsafe_queue<ImgPackage*>* pList;
	pList = &gListToShow;
	ImgPackage* imgPKG;
	
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

		if (av_read_frame(pFormatCtx, packet) < 0) {
			pCPthThis->msleep(5);
			continue;
		}

		if (packet->stream_index == videoindex) {
			
			int ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0) {
				av_packet_unref(packet);
				continue;
			}

			if (got_picture) {

				//if (pFormatCtx->streams[videoindex]->r_frame_rate.den != 0)
				//{
				//	int tRate = pFormatCtx->streams[videoindex]->r_frame_rate.num / pFormatCtx->streams[videoindex]->r_frame_rate.den;
				//	qDebug() << "tRate" << tRate << endl;
				//	if ((tRate > 0) && (tRate <= 120))
				//	{
				//		if (pCPthThis->playFrameRate != tRate)
				//		{
				//			pCPthThis->playFrameRate = tRate;
				//			
				//			//emit pCPthThis->signalFrameRateUpdate(tRate);
				//		}
				//	}
				//}
				
				sws_scale(img_convert_ctx,
					(const unsigned char* const*)pFrame->data,
					pFrame->linesize, 0, pCodecCtx->height,
					pFrameRGB->data, pFrameRGB->linesize);
				
				QImage tmpImg((uchar*)rgb_buffer, pCodecCtx->width, pCodecCtx->height, QImage::Format_RGB32);
				imgPKG = new ImgPackage;
				imgPKG->img = tmpImg.copy();

				ImgPackage* tmpPackegGiveUp = NULL;

				int maxFrameInListToShow;
				if (pCPthThis->realTimeMode == true)
				{
					maxFrameInListToShow = 5;  // just show the last picture;
				}
				else
				{
					maxFrameInListToShow = MAX_FRAME_IN_LIST_TO_SHOW;
				}

				int numret = pList->push(imgPKG, tmpPackegGiveUp, maxFrameInListToShow);

				if (tmpPackegGiveUp != NULL)
				{
					delete tmpPackegGiveUp;
				}

				av_packet_unref(packet);
			
			}
		}


	}

}
