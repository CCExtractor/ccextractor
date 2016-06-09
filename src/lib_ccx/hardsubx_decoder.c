#include "lib_ccx.h"
#include "utility.h"

#ifdef ENABLE_OCR
//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "allheaders.h"
#include "hardsubx.h"
#include "capi.h"

void _process_frame(AVFrame *frame, int width, int height, int index)
{
	if(index%25!=0)
		return;
	printf("frame : %04d\n", index);
	PIX *im;
	TessBaseAPI *handle;
	char *subtitle_text;
	im = pixCreate(width,height,32);
	int i,j;
	for(i=0;i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			int p=j*3+i*frame->linesize[0];
			int r=frame->data[0][p];
			int g=frame->data[0][p+1];
			int b=frame->data[0][p+2];
			//pixSetRGBPixel(im,j,i,r,g,b);
			float L,A,B;
			rgb2lab((float)r,(float)g,(float)b,&L,&A,&B);
			if(L>95) // TODO: Make this threshold a parameter and also automatically calculate it
				pixSetRGBPixel(im,j,i,255,255,255);
			else
				pixSetRGBPixel(im,j,i,0,0,0);
			//pixSetRGBPixel(im,j,i,r,g,b);
		}
	}

	handle = TessBaseAPICreate();
    if(TessBaseAPIInit3(handle, NULL, "eng") != 0)
        printf("Error initialising tesseract\n");

    TessBaseAPISetImage2(handle, im);
    if(TessBaseAPIRecognize(handle, NULL) != 0)
        printf("Error in Tesseract recognition\n");

    if((subtitle_text = TessBaseAPIGetUTF8Text(handle)) == NULL)
        printf("Error getting text\n");
    TessBaseAPIEnd(handle);
    TessBaseAPIDelete(handle);

    printf("Recognized text : \"%s\"\n", subtitle_text);

	char write_path[100];
	sprintf(write_path,"./ffmpeg-examples/frames/temp%04d.jpg",index);
	// printf("%s\n", write_path);
	pixWrite(write_path,im,IFF_JFIF_JPEG);

	pixDestroy(&im);
}

int hardsubx_process_frames_linear(struct lib_hardsubx_ctx *ctx)
{
	// Do an exhaustive linear search over the video
	int got_frame;
	int frame_number = 0;
	while(av_read_frame(ctx->format_ctx, &ctx->packet)>=0)
	{
		if(ctx->packet.stream_index == ctx->video_stream_id)
		{
			frame_number++;
			// printf("%d\n", frame_number);
			avcodec_decode_video2(ctx->codec_ctx, ctx->frame, &got_frame, &ctx->packet);
			if(got_frame)
			{
				sws_scale(
						ctx->sws_ctx,
						(uint8_t const * const *)ctx->frame->data,
						ctx->frame->linesize,
						0,
						ctx->codec_ctx->height,
						ctx->rgb_frame->data,
						ctx->rgb_frame->linesize
					);
				// Send the frame to other functions for processing
				_process_frame(ctx->rgb_frame,ctx->codec_ctx->width,ctx->codec_ctx->height,frame_number);
			}
		}
		av_packet_unref(&ctx->packet);
	}
}

int hardsubx_process_frames_binary(struct lib_hardsubx_ctx *ctx)
{
	// Do a binary search over the input video for faster processing
}

#endif