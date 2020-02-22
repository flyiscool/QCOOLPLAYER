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

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"
};

#define SIZE_READ_BUFF 32768

extern threadsafe_queue<UsbBuffPackage*> gListH264ToUDP;

static	CPThreadEncoderToUDP* pthEncoderToUDP;

//Callback
static int readH264ToUDPCallBack(void* opaque, uint8_t* buf, int buf_size)
{
	UsbBuffPackage* pBuff;
	int length = 0;
	int need = buf_size;

	static int cnt = 0;
	static int pktID = 0;
	int surplus = 0;

	do {



		while (gListH264ToUDP.empty())
		{
			pthEncoderToUDP->msleep(5);

			if (!(pthEncoderToUDP->IsRun())) {
				return length;
			}
		}

		gListH264ToUDP.try_front(pBuff);

		if (pktID != pBuff->packageID)  // package has been lost，so give up
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

			gListH264ToUDP.try_pop(pBuff);
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

				gListH264ToUDP.try_pop(pBuff);
				delete pBuff;
			}
		}


	} while (buf_size > 0);

	return length;
}


//
//void threadCPEncoderToUDP_main(CPThreadEncoderToUDP* pCPThreadEncoderToUDP)
//{
//	CPThreadEncoderToUDP* pCPthThis = pCPThreadEncoderToUDP;
//	pthEncoderToUDP = pCPThreadEncoderToUDP;
//	// init ffmpeg
//	av_register_all();
//	avformat_network_init();
//
//	AVInputFormat* pAVInputFmt = NULL;
//	AVOutputFormat* pAVOutputFmt = NULL;
//
//	AVFormatContext* pFormatCtx_In = avformat_alloc_context();
//	AVFormatContext* pFormatCtx_Out = avformat_alloc_context();
//
//	char errorbuf[1024] = { 0 };
//	bool openAgainFlag = true;
//	AVIOContext* avio;
//	unsigned char* aviobuffer;
//	do {
//
//		if (!(pCPthThis->IsRun())) {
//
//			if (avio) {
//				av_freep(&avio->buffer);
//				av_freep(&avio);
//			}
//			avformat_close_input(&pFormatCtx_In);
//			break;
//		}
//
//		//Init AVIOContext    read_buffer is a callback function
//		aviobuffer = (unsigned char*)av_malloc(SIZE_READ_BUFF);
//
//		avio = avio_alloc_context(aviobuffer, SIZE_READ_BUFF, 0, NULL, &readH264ToUDPCallBack, NULL, NULL);
//		
//		// Set buff to be the input for decoder
//		pFormatCtx_In->pb = avio;
//
//		int r = av_probe_input_buffer(pFormatCtx_In->pb, &pAVInputFmt, "", NULL, 0, 0);
//		if (r == 0)	// success and out
//		{
//			openAgainFlag = false;
//		}
//		else if (r > 0)
//		{
//			openAgainFlag = false;
//		}
//		else
//		{
//			av_strerror(r, errorbuf, sizeof(errorbuf));
//			qDebug() << "av_probe_input_buffer failed : " << r << QString(QLatin1String(errorbuf)) << endl;
//
//			if (r == -1094995529) //  Invalid data found when processing input
//			{
//				qDebug() << "try1" << endl;
//				openAgainFlag = true;
//				if (avio) {
//					av_freep(&avio->buffer);
//					av_freep(&avio);
//				}
//				pCPthThis->msleep(10);
//				continue;
//			}
//			else if (r == -572662307)
//			{
//				qDebug() << "try2" << endl;
//				openAgainFlag = true;
//				if (avio) {
//					av_freep(&avio->buffer);
//					av_freep(&avio);
//				}
//				pCPthThis->msleep(10);
//				continue;
//			}
//			else
//			{
//				qDebug() << "unknow error so return" << endl;
//				return;
//			}
//		}
//	} while (openAgainFlag);
//
//
//
//	int r = avformat_open_input(&pFormatCtx_In, NULL, pAVInputFmt, NULL);
//	//int r = avformat_open_input(&pFormatCtx, NULL, NULL, NULL);
//	if (r != 0)
//	{
//		qDebug() << "Couldn't open input stream." << endl;
//		return;
//	}
//
//	if (avformat_find_stream_info(pFormatCtx_In, NULL) < 0) {
//		qDebug() << "Couldn't find stream information." << endl;
//		return;
//	}
//
//	int videoindex = -1;
//	for (int i = 0; i < pFormatCtx_In->nb_streams; i++) {
//		if (pFormatCtx_In->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
//			videoindex = i;
//			break;
//		}
//	}
//
//	if (videoindex == -1) {
//		qDebug() << "Didn't find a video stream." << endl;
//		return;
//	}
//
//	avformat_alloc_output_context2(&pFormatCtx_Out, NULL, "h264", "rtmp://localhost/publishlive/livestream"); //RTMP
//	//avformat_alloc_output_context2(&pFormatCtx_Out, NULL, "flv", "rtp://233.233.233.233:6666"); //RTMP
//
//	while (1) {
//
//
//
//		if (!(pCPthThis->IsRun())) {
//			sws_freeContext(img_convert_ctx);
//
//			av_frame_free(&pFrameRGB);
//			av_frame_free(&pFrame);
//			av_packet_free(&packet);
//
//			avio_context_free(&avio);
//
//			avcodec_close(pCodecCtx);
//			avformat_close_input(&pFormatCtx);
//
//			break;
//		}
//
//
//
//		if (pCPthThis->playMode == SkyTxMode)
//		{
//
//			if (pList->size() >= MAX_FRAME_TO_SLEEP)
//			{
//				pCPthThis->msleep(50);
//				continue;
//			}
//		}
//
//		if (av_read_frame(pFormatCtx, packet) < 0) {
//			pCPthThis->msleep(100);
//			continue;
//		}
//
//		if (packet->stream_index == videoindex) {
//
//			int ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
//			if (ret < 0) {
//				av_packet_unref(packet);
//				continue;
//			}
//
//			if (got_picture) {
//				sws_scale(img_convert_ctx,
//					(const unsigned char* const*)pFrame->data,
//					pFrame->linesize, 0, pCodecCtx->height,
//					pFrameRGB->data, pFrameRGB->linesize);
//
//				//QImage tmpImg((uchar*)rgb_buffer, pCodecCtx->width, pCodecCtx->height, QImage::Format_RGB32);
//
//				QImage* tmpImg = new QImage((uchar*)rgb_buffer, pCodecCtx->width, pCodecCtx->height, QImage::Format_RGB32);
//
//				QImage* tmpPackegGiveUp = NULL;
//
//				int maxFrameInListToShow;
//				if (pCPthThis->playFrameRate == 120)
//				{
//					maxFrameInListToShow = 1;  // just show the last picture;
//				}
//				else
//				{
//					maxFrameInListToShow = 3;
//				}
//				if (pCPthThis->playMode == SkyTxMode)
//				{
//					int numret = pList->push(tmpImg, tmpPackegGiveUp, MAX_FRAME_IN_LIST_TO_SHOW);
//				}
//				else
//				{
//					int numret = pList->push(tmpImg, tmpPackegGiveUp, maxFrameInListToShow);
//				}
//
//				if (tmpPackegGiveUp != NULL)
//				{
//					delete tmpPackegGiveUp;
//				}
//
//				av_packet_unref(packet);
//
//			}
//		}
//
//
//	}
//
//}

void threadCPEncoderToUDP_main(CPThreadEncoderToUDP* pCPThreadEncoderToUDP)
{

	AVOutputFormat* ofmt = NULL;

	//输入对应一个AVFormatContext，输出对应一个AVFormatContext
	//（Input AVFormatContext and Output AVFormatContext）
	AVFormatContext* ifmt_ctx = NULL, * ofmt_ctx = NULL;
	AVPacket pkt;
	const char* in_filename, * out_filename;
	int ret, i;
	int videoindex = -1;
	int frame_index = 0;
	int64_t start_time = 0;
	//in_filename  = "cuc_ieschool.mov";
	//in_filename  = "cuc_ieschool.mkv";
	//in_filename  = "cuc_ieschool.ts";
	//in_filename  = "cuc_ieschool.mp4";
	//in_filename  = "bigbuckbunny_480x272.h264";
	in_filename = "Avatar_1920x1080@30Hz_3Mbps.264";//输入URL（Input file URL）
	//in_filename  = "shanghai03_p.h264";

	//out_filename = "rtmp://localhost/publishlive/livestream";//输出 URL（Output URL）[RTMP]
	out_filename = "udp://192.168.16.104:1234";//输出 URL（Output URL）[UDP]
	
	av_register_all();
	//Network
	avformat_network_init();
	//输入（Input）
	
	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
		qDebug() << "Could not open input file." <<endl;
		goto end;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		qDebug() << "Failed to retrieve input stream information" << endl;
		goto end;
	}

	for (i = 0; i < ifmt_ctx->nb_streams; i++)
		if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}

	av_dump_format(ifmt_ctx, 0, in_filename, 0);

	//输出（Output）

	//avformat_alloc_output_context2(&ofmt_ctx, NULL, "h264", out_filename); //RTMP
	avformat_alloc_output_context2(&ofmt_ctx, NULL, "h264", out_filename);//UDP
	if (!ofmt_ctx) {
		qDebug() << "Could not create output context" << endl;
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	
	ofmt = ofmt_ctx->oformat;
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		//根据输入流创建输出流（Create output AVStream according to input AVStream）
		AVStream* in_stream = ifmt_ctx->streams[i];
		AVStream* out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
		if (!out_stream) {
			qDebug() << "Failed allocating output stream" << endl;
			ret = AVERROR_UNKNOWN;
			goto end;
		}
		//复制AVCodecContext的设置（Copy the settings of AVCodecContext）
		AVCodecContext* codec_ctx = avcodec_alloc_context3(in_stream->codec->codec);
		ret = avcodec_parameters_to_context(codec_ctx, in_stream->codecpar);
		if (ret < 0) {
			qDebug() << "Failed to copy in_stream codecpar to codec context" << endl;
			goto end;
		}

		codec_ctx->codec_tag = 0;
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		ret = avcodec_parameters_from_context(out_stream->codecpar, codec_ctx);
		if (ret < 0) {
			qDebug() << "Failed to copy codec context to out_stream codecpar context" << endl;
			goto end;
		}
	}
	
	//Dump Format------------------
	av_dump_format(ofmt_ctx, 0, out_filename, 1);
	//打开输出URL（Open output URL）
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			qDebug() << "Could not open output URL "<< out_filename << endl;
			goto end;
		}
	}


	//写文件头（Write file header）
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		qDebug() << "Error occurred when opening output URL" << endl;
		goto end;
	}

	start_time = av_gettime();

	while (1) {
		AVStream* in_stream, * out_stream;
		//获取一个AVPacket（Get an AVPacket）
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0)
			break;

		

		//计算PTS
		//if (pkt.pts != AV_NOPTS_VALUE) {
		//	pkt.pts = av_rescale_q(pkt.pts, video_st->codec->time_base, video_st->time_base);
		//}
		//if (pkt.dts != AV_NOPTS_VALUE) {
		//	pkt.dts = av_rescale_q(pkt.dts, video_st->codec->time_base, video_st->time_base);
		//}

		//FIX：No PTS (Example: Raw H.264)
		//Simple Write PTS
		if (pkt.pts == AV_NOPTS_VALUE) {
			//Write PTS
			AVRational time_base1 = ifmt_ctx->streams[videoindex]->time_base;

			//Duration between 2 frames (us)
			int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(ifmt_ctx->streams[videoindex]->r_frame_rate);
			//Parameters
			pkt.pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
			pkt.dts = pkt.pts;
			pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
		}
	
		//Important:Delay
		//if (pkt.stream_index == videoindex) {
		//	AVRational time_base = ifmt_ctx->streams[videoindex]->time_base;
		//	AVRational time_base_q = { 1,AV_TIME_BASE };
		//	int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
		//	int64_t now_time = av_gettime() - start_time;
		//	if (pts_time > now_time)
		//		av_usleep(pts_time - now_time);

		//}
		Sleep(20);

		in_stream = ifmt_ctx->streams[pkt.stream_index];
		out_stream = ofmt_ctx->streams[pkt.stream_index];

		qDebug() << in_stream->time_base.den << out_stream->time_base.den << endl;

		///* copy packet */
		////转换PTS/DTS（Convert PTS/DTS）
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
	
		//Print to Screen
		if (pkt.stream_index == videoindex) {
			qDebug() << "Send  !! video frames to output URL"<< frame_index << endl;
			frame_index++;
		}
	
		//ret = av_write_frame(ofmt_ctx, &pkt);
		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);

		if (ret < 0) {
			qDebug() << "Error muxing packet" << frame_index << endl;
			break;
		}

		av_free_packet(&pkt);

	}

	//写文件尾（Write file trailer）
	av_write_trailer(ofmt_ctx);

end:
	avformat_close_input(&ifmt_ctx);
	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);

	avformat_free_context(ofmt_ctx);
	
	if (ret < 0 && ret != AVERROR_EOF) {
		qDebug() << "Error occurred." << endl;
		return;
}
return;
}