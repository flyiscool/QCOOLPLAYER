#pragma once

#include "cpThread.h"




struct buffer_data {
	uint8_t* ptr;
	size_t size; ///< size left in the buffer
};


void threadCPDecoderFfmpeg_main(CPThreadDecoderFfmpeg* pCPThreadDecoderFfmpeg);

