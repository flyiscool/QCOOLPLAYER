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



static FILE* g_fpVedio = NULL;
extern threadsafe_queue<QImage*> gListToShow;

#define MAX_FRAME_TO_SLEEP	5
#define MAX_FRAME_IN_LIST_TO_SHOW	30

//Callback
static int readBuffCallBack(void* opaque, uint8_t* buf, int buf_size) 
{

	int true_size = fread(buf, 1, buf_size, g_fpVedio);
	
	if (feof(g_fpVedio)) {
		rewind(g_fpVedio);
	}
	
	return true_size;
}



#define SIZE_READ_BUFF 32768
#define OUTPUT_YUV420P 0

void threadCPDecoderFfmpeg_main(CPThreadDecoderFfmpeg* pCPThreadDecoderFfmpeg)
{
	// init ffmpeg
	av_register_all();
	avformat_network_init();

	// open file
	QString fileName_H264 = pCPThreadDecoderFfmpeg->getFileName();
	QByteArray temp = fileName_H264.toLatin1();
	char* pFileName_H264 = temp.data();

	g_fpVedio = fopen(pFileName_H264, "rb+");
	if (!g_fpVedio) {
		qDebug() << "Can't open video file!" << endl;
		return;
	}

	AVFormatContext* pFormatCtx = avformat_alloc_context();

	//Init AVIOContext    read_buffer is a callback function
	unsigned char* aviobuffer = (unsigned char*)av_malloc(SIZE_READ_BUFF);

	// Set the call back
	AVIOContext* avio = avio_alloc_context(aviobuffer, SIZE_READ_BUFF, 0, NULL, readBuffCallBack, NULL, NULL);

	// Set buff to be the input for decoder
	pFormatCtx->pb = avio;


	//if (avformat_open_input(&pFormatCtx, pFileName_H264, NULL, NULL) != 0) {
	//	qDebug() << "Couldn't open input stream." << endl;
	//	return;
	//}

	if (avformat_open_input(&pFormatCtx, NULL, NULL, NULL) != 0) {
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

		if (!(pCPThreadDecoderFfmpeg->IsRun())) {
			sws_freeContext(img_convert_ctx);

			av_frame_free(&pFrameRGB);
			av_frame_free(&pFrame);
			av_packet_free(&packet);

			avio_context_free(&avio);

			avcodec_close(pCodecCtx);
			avformat_close_input(&pFormatCtx);

			break;
		}

		if (pList->size() >= MAX_FRAME_TO_SLEEP)
		{
			pCPThreadDecoderFfmpeg->msleep(50);
			continue;
		}

		if (av_read_frame(pFormatCtx, packet) < 0) {
			pCPThreadDecoderFfmpeg->msleep(100);
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
				int numret = pList->push(tmpImg, tmpPackegGiveUp, MAX_FRAME_IN_LIST_TO_SHOW);
				if (tmpPackegGiveUp != NULL)
				{
					delete tmpPackegGiveUp;
				}
				av_packet_unref(packet);
			
			}
		}


	}

}
