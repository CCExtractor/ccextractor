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

void _process_frame(AVFrame *frame, int width, int height, int index, PIX *prev_im)
{
	if(index%5!=0)
		return;
	printf("frame : %04d\n", index);
	PIX *im;
	PIX *edge_im;
	PIX *lum_im;
	PIX *feature_img;
	PIX *mov_im;
	TessBaseAPI *handle;
	char *subtitle_text;
	im = pixCreate(width,height,32);
	lum_im = pixCreate(width,height,32);
	edge_im = pixCreate(width,height,8);
	mov_im = pixCreate(width,height,32);
	feature_img = pixCreate(width,height,32);
	int i,j;
	for(i=(3*height)/4;i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			int p=j*3+i*frame->linesize[0];
			int r=frame->data[0][p];
			int g=frame->data[0][p+1];
			int b=frame->data[0][p+2];
			pixSetRGBPixel(im,j,i,r,g,b);
			float L,A,B;
			rgb2lab((float)r,(float)g,(float)b,&L,&A,&B);
			if(L>95) // TODO: Make this threshold a parameter and also automatically calculate it
				pixSetRGBPixel(lum_im,j,i,255,255,255);
			else
				pixSetRGBPixel(lum_im,j,i,0,0,0);
			int r_prev, g_prev, b_prev;
			pixGetRGBPixel(prev_im,j,i,&r_prev,&g_prev,&b_prev);
			//printf("%d %d %d\n", (int)r_prev,(int)g_prev,(int)b_prev);
			if(abs(r-r_prev)<5&&abs(g-g_prev)<5&&abs(b-b_prev)<5)
			{
				//printf("%d %d %d , %d %d %d\n", r,g,b,r_prev,g_prev,b_prev);
				pixSetRGBPixel(mov_im,j,i,255,255,255);
			}
		}
	}

	PIX *pixd,*pixs,*edge_im_2;
	pixd = pixCreate(width,height,1);
	pixs = pixCreate(width,height,8);
	edge_im_2 = pixCreate(width,height,8);

	edge_im = pixConvertRGBToGray(im,0.0,0.0,0.0);
	pixCopy(pixs,edge_im);
	pixCopy(edge_im_2,edge_im);
	edge_im = pixSobelEdgeFilter(edge_im, L_VERTICAL_EDGES);
	edge_im = pixDilateGray(edge_im, 31, 11);
	// edge_im = pixDilateGray(edge_im, 5, 5);
	// edge_im = pixDilateGray(edge_im, 3, 1);
	edge_im = pixThresholdToBinary(edge_im,50);
	//edge_im = pixConvert1To8(NULL,pixThresholdToBinary(edge_im, 50),0,255);

	// edge_im_2 = pixDilateGray(edge_im_2, 5, 5);
	edge_im_2 = pixSobelEdgeFilter(edge_im_2, L_ALL_EDGES);
	edge_im_2 = pixDilateGray(edge_im_2, 5, 5);
	edge_im_2 = pixThresholdToBinary(edge_im_2,50);
	// for(i=(3*height)/4;i<height;i++)
	// {
	// 	for(j=0;j<width;j++)
	// 	{
	// 		unsigned int pixelval;
	// 		pixGetPixel(edge_im,j,i,&pixelval);
	// 		if(pixelval > 0)
	// 		{
	// 			pixSetPixel(edge_im_2,j,i,1);
	// 		}
	// 	}
	// }
	
	pixSauvolaBinarize(pixs, 3, 0.01, 1, NULL, NULL, NULL, &pixd);

	for(i=(3*height)/4;i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			unsigned int pixelval,pixelval1,pixelval2;
			pixGetPixel(edge_im,j,i,&pixelval);
			pixGetPixel(edge_im_2,j,i,&pixelval1);
			pixGetPixel(pixd,j,i,&pixelval2);
			if(pixelval2 == 0  && pixelval1 == 0 && pixelval == 0)
			{
				pixSetRGBPixel(feature_img,j,i,255,255,255);
			}
		}
	}



	

	// // TESSERACT OCR FOR THE FRAME HERE
	// // TODO: Move to ctx
	// handle = TessBaseAPICreate();
 //    if(TessBaseAPIInit3(handle, NULL, "eng") != 0)
 //        printf("Error initialising tesseract\n");

 //    TessBaseAPISetImage2(handle, lum_im);
 //    if(TessBaseAPIRecognize(handle, NULL) != 0)
 //        printf("Error in Tesseract recognition\n");

 //    if((subtitle_text = TessBaseAPIGetUTF8Text(handle)) == NULL)
 //        printf("Error getting text\n");
 //    TessBaseAPIEnd(handle);
 //    TessBaseAPIDelete(handle);





    // printf("Recognized text : \"%s\"\n", subtitle_text);

	char write_path[100];
	sprintf(write_path,"./ffmpeg-examples/frames/temp%04d.jpg",index);
	// printf("%s\n", write_path);
	pixWrite(write_path,feature_img,IFF_JFIF_JPEG);

	pixCopy(prev_im,im);

	pixDestroy(&lum_im);
	pixDestroy(&edge_im);
	pixDestroy(&mov_im);
	pixDestroy(&feature_img);
	pixDestroy(&im);
}

void _display_frame(AVFrame *frame, int width, int height, int timestamp)
{
	PIX *im;
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
			pixSetRGBPixel(im,j,i,r,g,b);
		}
	}


	char write_path[100];
	sprintf(write_path,"./ffmpeg-examples/frames/temp%04d.jpg",timestamp);

	pixWrite(write_path,im,IFF_JFIF_JPEG);
}

int hardsubx_process_frames_linear(struct lib_hardsubx_ctx *ctx, struct encoder_ctx *enc_ctx)
{
	// Do an exhaustive linear search over the video
	int got_frame;
	int frame_number = 0;
	PIX *prev_im;
	prev_im = pixCreate(ctx->codec_ctx->width,ctx->codec_ctx->height,32);
	while(av_read_frame(ctx->format_ctx, &ctx->packet)>=0)
	{
		if(ctx->packet.stream_index == ctx->video_stream_id)
		{
			frame_number++;
			// printf("%d\n", frame_number);
			avcodec_decode_video2(ctx->codec_ctx, ctx->frame, &got_frame, &ctx->packet);
			if(got_frame)
			{
				// sws_scale is used to convert the pixel format to RGB24 from all other cases
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
				_process_frame(ctx->rgb_frame,ctx->codec_ctx->width,ctx->codec_ctx->height,frame_number,prev_im);
			}
		}
		av_packet_unref(&ctx->packet);
	}
}

int hardsubx_process_frames_binary(struct lib_hardsubx_ctx *ctx)
{
	// Do a binary search over the input video for faster processing
	printf("Duration: %d\n", (int)ctx->format_ctx->duration);
	int got_frame;
	int seconds_time = 0;
	for(seconds_time=0;seconds_time<20;seconds_time++){
	int64_t seek_time = (int64_t)(seconds_time*AV_TIME_BASE);
	seek_time = av_rescale_q(seek_time, AV_TIME_BASE_Q, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);

	int ret = av_seek_frame(ctx->format_ctx, ctx->video_stream_id, seek_time, AVSEEK_FLAG_BACKWARD);
	printf("%d\n", ret);
	// if(ret < 0)
	// {
	// 	printf("seeking back\n");
	// 	ret = av_seek_frame(ctx->format_ctx, -1, seek_time, AVSEEK_FLAG_BACKWARD);
	// }
	if(ret >= 0)
	{
		while(av_read_frame(ctx->format_ctx, &ctx->packet)>=0)
		{
			if(ctx->packet.stream_index == ctx->video_stream_id)
			{
				avcodec_decode_video2(ctx->codec_ctx, ctx->frame, &got_frame, &ctx->packet);
				if(got_frame)
				{
					// printf("%d\n", seek_time);
					if(ctx->packet.pts < seek_time)
						continue;
					printf("GOT FRAME: %d\n",ctx->packet.pts);
					// sws_scale is used to convert the pixel format to RGB24 from all other cases
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
					_display_frame(ctx->rgb_frame,ctx->codec_ctx->width,ctx->codec_ctx->height,seconds_time);
					break;
				}
			}
		}
	}
	else
	{
		printf("Seeking to timestamp failed\n");
	}
	}
}

#endif