#include "stdafx.h"
#include <QtWidgets/QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>
#include <QThread>
#include <queue>
#include <time.h>
#include "cpThread.h"



extern "C"
{

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "SDL.h"

};





//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

int thread_exit = 0;
int thread_pause = 0;


int sfp_refresh_thread(void* opaque)
{
	thread_exit = 0;
	thread_pause = 0;

	while (!thread_exit) {
		if (!thread_pause) {
			SDL_Event event;
			event.type = SFM_REFRESH_EVENT;
			SDL_PushEvent(&event);
		}
		SDL_Delay(33);
	}

	thread_exit = 0;
	thread_pause = 0;

	//Break
	SDL_Event event;
	event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}


void threadCPDecoderFfmpeg_main(CPThreadDecoderFfmpeg* pCPThreadDecoderFfmpeg)
{
	QString fileName_H264 = pCPThreadDecoderFfmpeg->getFileName();
	QByteArray temp = fileName_H264.toLatin1(); 
	char* pfileName_H264 = temp.data();

	av_register_all();
	avformat_network_init();

	AVFormatContext* pFormatCtx = avformat_alloc_context();

	QString runPath = QCoreApplication::applicationDirPath();
	
	if (avformat_open_input(&pFormatCtx, pfileName_H264, NULL, NULL) != 0) {
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
	AVFrame* pFrameYUV = av_frame_alloc();

	unsigned char* out_buffer = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
		AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
	
	av_dump_format(pFormatCtx, 0, pfileName_H264, 0);

	struct SwsContext* img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	


	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		qDebug() << "Could not initialize SDL -" << SDL_GetError() <<endl;
		return;
	}

	//SDL 2.0 Support for multiple windows
	int screen_w = pCodecCtx->width;
	int screen_h = pCodecCtx->height;
	SDL_Window* screen = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h, SDL_WINDOW_OPENGL);

	if (!screen) {
		qDebug() << "SDL: could not create window - exiting:%s" << SDL_GetError() << endl;
		return;
	}

	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
	//IYUV: Y + U + V  (3 planes)
	//YV12: Y + V + U  (3 planes)
	SDL_Texture* sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);
	
	SDL_Rect sdlRect;
	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = screen_w/2;
	sdlRect.h = screen_h/2;

	AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));

	SDL_Thread* video_tid = SDL_CreateThread(sfp_refresh_thread, NULL, NULL);




	SDL_Event event;

	while (1){

		//Wait
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT) {
			while (1) {
				if (av_read_frame(pFormatCtx, packet) < 0)
					thread_exit = 1;

				if (packet->stream_index == videoindex)
					break;
			}

			int got_picture;
			int ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0) {
				qDebug() << "Decode Error." << endl;
				return;
			}
			if (got_picture) {
				sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
				//SDL---------------------------
				SDL_UpdateTexture(sdlTexture, NULL, pFrameYUV->data[0], pFrameYUV->linesize[0]);
				SDL_RenderClear(sdlRenderer);
				//SDL_RenderCopy( sdlRenderer, sdlTexture, &sdlRect, &sdlRect );  
				SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
				SDL_RenderPresent(sdlRenderer);
				//SDL End-----------------------
			}
			av_free_packet(packet);
		}
		else if (event.type == SDL_KEYDOWN) {
			//Pause
			if (event.key.keysym.sym == SDLK_SPACE)
				thread_pause = !thread_pause;
		}
		else if (event.type == SDL_QUIT) {
			thread_exit = 1;
		}
		else if (event.type == SFM_BREAK_EVENT) {
			break;
		}




		if (!(pCPThreadDecoderFfmpeg->IsRun()))
		{
			sws_freeContext(img_convert_ctx);

			SDL_Quit();

			av_frame_free(&pFrameYUV);
			av_frame_free(&pFrame);
			avcodec_close(pCodecCtx);
			avformat_close_input(&pFormatCtx);

			break;
		}	
	}

}
