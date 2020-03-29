#include "lib_ccx.h"
#include "utility.h"

#ifdef ENABLE_HARDSUBX
//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <leptonica/allheaders.h>
#include <tesseract/capi.h>
#include "hardsubx.h"

char *_process_frame_white_basic(struct lib_hardsubx_ctx *ctx, AVFrame *frame, int width, int height, int index)
{
	//printf("frame : %04d\n", index);
	PIX *im;
	PIX *gray_im;
	PIX *sobel_edge_im;
	PIX *dilate_gray_im;
	PIX *edge_im;
	PIX *lum_im;
	PIX *feat_im;
	char *subtitle_text;

	subtitle_text = NULL;
	im = pixCreate(width, height, 32);
	lum_im = pixCreate(width, height, 32);
	feat_im = pixCreate(width, height, 32);

	for (int i = (3 * height) / 4; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			int p = j * 3 + i * frame->linesize[0];
			int r = frame->data[0][p];
			int g = frame->data[0][p + 1];
			int b = frame->data[0][p + 2];
			pixSetRGBPixel(im, j, i, r, g, b);
			float L, A, B;
			rgb_to_lab((float)r, (float)g, (float)b, &L, &A, &B);
			if (L > ctx->lum_thresh)
				pixSetRGBPixel(lum_im, j, i, 255, 255, 255);
			else
				pixSetRGBPixel(lum_im, j, i, 0, 0, 0);
		}
	}

	//Handle the edge image
	gray_im = pixConvertRGBToGray(im, 0.0, 0.0, 0.0);
	sobel_edge_im = pixSobelEdgeFilter(gray_im, L_VERTICAL_EDGES);
	dilate_gray_im = pixDilateGray(sobel_edge_im, 21, 11);
	edge_im = pixThresholdToBinary(dilate_gray_im, 50);

	for (int i = 3 * (height / 4); i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			unsigned int p1, p2, p3;
			pixGetPixel(edge_im, j, i, &p1);
			// pixGetPixel(pixd,j,i,&p2);
			pixGetPixel(lum_im, j, i, &p3);
			if (p1 == 0 && p3 > 0)
				pixSetRGBPixel(feat_im, j, i, 255, 255, 255);
			else
				pixSetRGBPixel(feat_im, j, i, 0, 0, 0);
		}
	}

	if (ctx->detect_italics)
	{
		ctx->ocr_mode = HARDSUBX_OCRMODE_WORD;
	}

	// TESSERACT OCR FOR THE FRAME HERE
	switch (ctx->ocr_mode)
	{
		case HARDSUBX_OCRMODE_WORD:
			if (ctx->conf_thresh > 0)
				subtitle_text = get_ocr_text_wordwise_threshold(ctx, lum_im, ctx->conf_thresh);
			else
				subtitle_text = get_ocr_text_wordwise(ctx, lum_im);
			break;
		case HARDSUBX_OCRMODE_LETTER:
			if (ctx->conf_thresh > 0)
				subtitle_text = get_ocr_text_letterwise_threshold(ctx, lum_im, ctx->conf_thresh);
			else
				subtitle_text = get_ocr_text_letterwise(ctx, lum_im);
			break;
		case HARDSUBX_OCRMODE_FRAME:
			if (ctx->conf_thresh > 0)
				subtitle_text = get_ocr_text_simple_threshold(ctx, lum_im, ctx->conf_thresh);
			else
				subtitle_text = get_ocr_text_simple(ctx, lum_im);
			break;
		default:
			fatal(EXIT_MALFORMED_PARAMETER, "Invalid OCR Mode");
	}

	pixDestroy(&im);
	pixDestroy(&gray_im);
	pixDestroy(&sobel_edge_im);
	pixDestroy(&dilate_gray_im);
	pixDestroy(&edge_im);
	pixDestroy(&lum_im);
	pixDestroy(&feat_im);

	return subtitle_text;
}

char *_process_frame_color_basic(struct lib_hardsubx_ctx *ctx, AVFrame *frame, int width, int height, int index)
{
	PIX *im;
	PIX *hue_im;
	PIX *gray_im;
	PIX *sobel_edge_im;
	PIX *dilate_gray_im;
	PIX *edge_im;
	PIX *gray_im_2;
	PIX *edge_im_2;
	PIX *pixd;
	PIX *feat_im;
	char *subtitle_text = NULL;

	im = pixCreate(width, height, 32);
	hue_im = pixCreate(width, height, 32);
	subtitle_text = NULL;

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			int p = j * 3 + i * frame->linesize[0];
			int r = frame->data[0][p];
			int g = frame->data[0][p + 1];
			int b = frame->data[0][p + 2];
			pixSetRGBPixel(im, j, i, r, g, b);
			float H, S, V;
			rgb_to_hsv((float)r, (float)g, (float)b, &H, &S, &V);
			if (fabsf(H - ctx->hue) < 20)
			{
				pixSetRGBPixel(hue_im, j, i, r, g, b);
			}
		}
	}

	gray_im = pixConvertRGBToGray(im, 0.0, 0.0, 0.0);
	sobel_edge_im = pixSobelEdgeFilter(gray_im, L_VERTICAL_EDGES);
	dilate_gray_im = pixDilateGray(sobel_edge_im, 21, 1);
	edge_im = pixThresholdToBinary(dilate_gray_im, 50);

	gray_im_2 = pixConvertRGBToGray(hue_im, 0.0, 0.0, 0.0);
	edge_im_2 = pixDilateGray(gray_im_2, 5, 5);

	pixSauvolaBinarize(gray_im_2, 15, 0.3, 1, NULL, NULL, NULL, &pixd);

	feat_im = pixCreate(width, height, 32);
	for (int i = 3 * (height / 4); i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			unsigned int p1, p2, p3, p4;
			pixGetPixel(edge_im, j, i, &p1);
			pixGetPixel(pixd, j, i, &p2);
			// pixGetPixel(hue_im,j,i,&p3);
			pixGetPixel(edge_im_2, j, i, &p4);
			if (p1 == 0 && p2 == 0 && p4 > 0) //if(p4>0&&p1==0)//if(p2==0&&p1==0&&p3>0)
			{
				pixSetRGBPixel(feat_im, j, i, 255, 255, 255);
			}
		}
	}

	if (ctx->detect_italics)
	{
		ctx->ocr_mode = HARDSUBX_OCRMODE_WORD;
	}

	// TESSERACT OCR FOR THE FRAME HERE
	switch (ctx->ocr_mode)
	{
		case HARDSUBX_OCRMODE_WORD:
			if (ctx->conf_thresh > 0)
				subtitle_text = get_ocr_text_wordwise_threshold(ctx, feat_im, ctx->conf_thresh);
			else
				subtitle_text = get_ocr_text_wordwise(ctx, feat_im);
			break;
		case HARDSUBX_OCRMODE_LETTER:
			if (ctx->conf_thresh > 0)
				subtitle_text = get_ocr_text_letterwise_threshold(ctx, feat_im, ctx->conf_thresh);
			else
				subtitle_text = get_ocr_text_letterwise(ctx, feat_im);
			break;
		case HARDSUBX_OCRMODE_FRAME:
			if (ctx->conf_thresh > 0)
				subtitle_text = get_ocr_text_simple_threshold(ctx, feat_im, ctx->conf_thresh);
			else
				subtitle_text = get_ocr_text_simple(ctx, feat_im);
			break;
		default:
			fatal(EXIT_MALFORMED_PARAMETER, "Invalid OCR Mode");
	}

	pixDestroy(&im);
	pixDestroy(&hue_im);
	pixDestroy(&gray_im);
	pixDestroy(&sobel_edge_im);
	pixDestroy(&dilate_gray_im);
	pixDestroy(&edge_im);
	pixDestroy(&gray_im_2);
	pixDestroy(&edge_im_2);
	pixDestroy(&pixd);
	pixDestroy(&feat_im);

	return subtitle_text;
}

void _display_frame(struct lib_hardsubx_ctx *ctx, AVFrame *frame, int width, int height, int timestamp)
{
	// Debug: Display the frame after processing
	PIX *im;
	PIX *hue_im;
	PIX *gray_im;
	PIX *sobel_edge_im;
	PIX *dilate_gray_im;
	PIX *edge_im;
	PIX *gray_im_2;
	PIX *edge_im_2;
	PIX *pixd;
	PIX *feat_im;

	im = pixCreate(width, height, 32);
	hue_im = pixCreate(width, height, 32);

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			int p = j * 3 + i * frame->linesize[0];
			int r = frame->data[0][p];
			int g = frame->data[0][p + 1];
			int b = frame->data[0][p + 2];
			pixSetRGBPixel(im, j, i, r, g, b);
			float H, S, V;
			rgb_to_hsv((float)r, (float)g, (float)b, &H, &S, &V);
			if (fabsf(H - ctx->hue) < 20)
			{
				pixSetRGBPixel(hue_im, j, i, r, g, b);
			}
		}
	}

	gray_im = pixConvertRGBToGray(im, 0.0, 0.0, 0.0);
	sobel_edge_im = pixSobelEdgeFilter(gray_im, L_VERTICAL_EDGES);
	dilate_gray_im = pixDilateGray(sobel_edge_im, 21, 1);
	edge_im = pixThresholdToBinary(dilate_gray_im, 50);

	gray_im_2 = pixConvertRGBToGray(hue_im, 0.0, 0.0, 0.0);
	edge_im_2 = pixDilateGray(gray_im_2, 5, 5);

	pixSauvolaBinarize(gray_im_2, 15, 0.3, 1, NULL, NULL, NULL, &pixd);

	feat_im = pixCreate(width, height, 32);
	for (int i = 3 * (height / 4); i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			unsigned int p1, p2, p3, p4;
			pixGetPixel(edge_im, j, i, &p1);
			pixGetPixel(pixd, j, i, &p2);
			// pixGetPixel(hue_im,j,i,&p3);
			pixGetPixel(edge_im_2, j, i, &p4);
			if (p1 == 0 && p2 == 0 && p4 > 0) //if(p4>0&&p1==0)//if(p2==0&&p1==0&&p3>0)
			{
				pixSetRGBPixel(feat_im, j, i, 255, 255, 255);
			}
		}
	}

	// char *txt=NULL;
	// txt = get_ocr_text_simple(ctx, feat_im);
	// txt=get_ocr_text_wordwise_threshold(ctx, feat_im, ctx->conf_thresh);
	// if(txt != NULL)printf("%s\n", txt);

	pixDestroy(&im);
	pixDestroy(&hue_im);
	pixDestroy(&gray_im);
	pixDestroy(&sobel_edge_im);
	pixDestroy(&dilate_gray_im);
	pixDestroy(&edge_im);
	pixDestroy(&gray_im_2);
	pixDestroy(&edge_im_2);
	pixDestroy(&pixd);
	pixDestroy(&feat_im);
}

char *_process_frame_tickertext(struct lib_hardsubx_ctx *ctx, AVFrame *frame, int width, int height, int index)
{
	PIX *im;
	PIX *gray_im;
	PIX *sobel_edge_im;
	PIX *dilate_gray_im;
	PIX *edge_im;
	PIX *lum_im;
	PIX *feat_im;
	char *subtitle_text;

	subtitle_text = NULL;
	im = pixCreate(width, height, 32);
	lum_im = pixCreate(width, height, 32);
	feat_im = pixCreate(width, height, 32);

	for (int i = (92 * height) / 100; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			int p = j * 3 + i * frame->linesize[0];
			int r = frame->data[0][p];
			int g = frame->data[0][p + 1];
			int b = frame->data[0][p + 2];
			pixSetRGBPixel(im, j, i, r, g, b);
			float L, A, B;
			rgb_to_lab((float)r, (float)g, (float)b, &L, &A, &B);
			if (L > ctx->lum_thresh)
				pixSetRGBPixel(lum_im, j, i, 255, 255, 255);
			else
				pixSetRGBPixel(lum_im, j, i, 0, 0, 0);
		}
	}

	//Handle the edge image
	gray_im = pixConvertRGBToGray(im, 0.0, 0.0, 0.0);
	sobel_edge_im = pixSobelEdgeFilter(gray_im, L_VERTICAL_EDGES);
	dilate_gray_im = pixDilateGray(sobel_edge_im, 21, 11);
	edge_im = pixThresholdToBinary(dilate_gray_im, 50);

	for (int i = 92 * (height / 100); i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			unsigned int p1, p2, p3;
			pixGetPixel(edge_im, j, i, &p1);
			// pixGetPixel(pixd,j,i,&p2);
			pixGetPixel(lum_im, j, i, &p3);
			if (p1 == 0 && p3 > 0)
				pixSetRGBPixel(feat_im, j, i, 255, 255, 255);
			else
				pixSetRGBPixel(feat_im, j, i, 0, 0, 0);
		}
	}

	// Tesseract OCR for the ticker text here
	subtitle_text = get_ocr_text_simple(ctx, lum_im);
	char write_path[100];
	sprintf(write_path, "./lum_im%04d.jpg", index);
	pixWrite(write_path, lum_im, IFF_JFIF_JPEG);
	sprintf(write_path, "./im%04d.jpg", index);
	pixWrite(write_path, im, IFF_JFIF_JPEG);

	pixDestroy(&im);
	pixDestroy(&gray_im);
	pixDestroy(&sobel_edge_im);
	pixDestroy(&dilate_gray_im);
	pixDestroy(&edge_im);
	pixDestroy(&lum_im);
	pixDestroy(&feat_im);

	return subtitle_text;
}

int hardsubx_process_frames_tickertext(struct lib_hardsubx_ctx *ctx, struct encoder_ctx *enc_ctx)
{
	// Search for ticker text at the bottom of the screen, such as in Russia TV1 or stock prices
	int got_frame;
	int cur_sec = 0, total_sec, progress;
	int frame_number = 0;
	char *ticker_text = NULL;

	while (av_read_frame(ctx->format_ctx, &ctx->packet) >= 0)
	{
		if (ctx->packet.stream_index == ctx->video_stream_id)
		{
			frame_number++;
			//Decode the video stream packet
			avcodec_decode_video2(ctx->codec_ctx, ctx->frame, &got_frame, &ctx->packet);
			if (got_frame && frame_number % 1000 == 0)
			{
				// sws_scale is used to convert the pixel format to RGB24 from all other cases
				sws_scale(
				    ctx->sws_ctx,
				    (uint8_t const *const *)ctx->frame->data,
				    ctx->frame->linesize,
				    0,
				    ctx->codec_ctx->height,
				    ctx->rgb_frame->data,
				    ctx->rgb_frame->linesize);

				ticker_text = _process_frame_tickertext(ctx, ctx->rgb_frame, ctx->codec_ctx->width, ctx->codec_ctx->height, frame_number);
				printf("frame_number: %d\n", frame_number);

				if (strlen(ticker_text) > 0)
					printf("%s\n", ticker_text);

				cur_sec = (int)convert_pts_to_s(ctx->packet.pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
				total_sec = (int)convert_pts_to_s(ctx->format_ctx->duration, AV_TIME_BASE_Q);
				progress = (cur_sec * 100) / total_sec;
				activity_progress(progress, cur_sec / 60, cur_sec % 60);
			}
		}
		av_packet_unref(&ctx->packet);
	}
	activity_progress(100, cur_sec / 60, cur_sec % 60);
	return 0;
}

void hardsubx_process_frames_linear(struct lib_hardsubx_ctx *ctx, struct encoder_ctx *enc_ctx)
{
	// Do an exhaustive linear search over the video

	int prev_sub_encoded = 1; // Previous seen subtitle encoded or not
	int got_frame;
	int dist = 0;
	int cur_sec = 0, total_sec, progress;
	int frame_number = 0;
	int64_t prev_begin_time = 0, prev_end_time = 0; // Begin and end time of previous seen subtitle
	int64_t prev_packet_pts = 0;
	char *subtitle_text = NULL;	 // Subtitle text of current frame
	char *prev_subtitle_text = NULL; // Previously seen subtitle text

	while (av_read_frame(ctx->format_ctx, &ctx->packet) >= 0)
	{
		if (ctx->packet.stream_index == ctx->video_stream_id)
		{
			frame_number++;

			//Decode the video stream packet
			avcodec_decode_video2(ctx->codec_ctx, ctx->frame, &got_frame, &ctx->packet);

			if (got_frame && frame_number % 25 == 0)
			{
				float diff = (float)convert_pts_to_ms(ctx->packet.pts - prev_packet_pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
				if (fabsf(diff) < 1000 * ctx->min_sub_duration) //If the minimum duration of a subtitle line is exceeded, process packet
					continue;

				// sws_scale is used to convert the pixel format to RGB24 from all other cases
				sws_scale(
				    ctx->sws_ctx,
				    (uint8_t const *const *)ctx->frame->data,
				    ctx->frame->linesize,
				    0,
				    ctx->codec_ctx->height,
				    ctx->rgb_frame->data,
				    ctx->rgb_frame->linesize);

				// Send the frame to other functions for processing
				if (ctx->subcolor == HARDSUBX_COLOR_WHITE)
				{
					subtitle_text = _process_frame_white_basic(ctx, ctx->rgb_frame, ctx->codec_ctx->width, ctx->codec_ctx->height, frame_number);
				}
				else
				{
					subtitle_text = _process_frame_color_basic(ctx, ctx->rgb_frame, ctx->codec_ctx->width, ctx->codec_ctx->height, frame_number);
				}
				_display_frame(ctx, ctx->rgb_frame, ctx->codec_ctx->width, ctx->codec_ctx->height, frame_number);

				cur_sec = (int)convert_pts_to_s(ctx->packet.pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
				total_sec = (int)convert_pts_to_s(ctx->format_ctx->duration, AV_TIME_BASE_Q);
				progress = (cur_sec * 100) / total_sec;
				activity_progress(progress, cur_sec / 60, cur_sec % 60);

				if ((!subtitle_text && !prev_subtitle_text) || (subtitle_text && !strlen(subtitle_text) && !prev_subtitle_text))
				{
					prev_end_time = convert_pts_to_ms(ctx->packet.pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
				}

				if (subtitle_text)
				{
					char *double_enter = strstr(subtitle_text, "\n\n");
					if (double_enter != NULL)
						*(double_enter) = '\0';
				}

				if (!prev_sub_encoded && prev_subtitle_text)
				{
					if (subtitle_text)
					{
						dist = edit_distance(subtitle_text, prev_subtitle_text, (int)strlen(subtitle_text), (int)strlen(prev_subtitle_text));
						if (dist < (0.2 * MIN(strlen(subtitle_text), strlen(prev_subtitle_text))))
						{
							dist = -1;
							subtitle_text = NULL;
							prev_end_time = convert_pts_to_ms(ctx->packet.pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
						}
					}
					if (dist != -1)
					{
						add_cc_sub_text(ctx->dec_sub, prev_subtitle_text, prev_begin_time, prev_end_time, "", "BURN", CCX_ENC_UTF_8);
						encode_sub(enc_ctx, ctx->dec_sub);
						prev_begin_time = prev_end_time + 1;
						prev_subtitle_text = NULL;
						prev_sub_encoded = 1;
						prev_end_time = convert_pts_to_ms(ctx->packet.pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
						if (subtitle_text)
						{
							prev_subtitle_text = strdup(subtitle_text);
							prev_sub_encoded = 0;
						}
					}
					dist = 0;
				}

				// if(ctx->conf_thresh > 0)
				// {
				// 	if(ctx->cur_conf >= ctx->prev_conf)
				// 	{
				// 		prev_subtitle_text = strdup(subtitle_text);
				// 		ctx->prev_conf = ctx->cur_conf;
				// 	}
				// }
				// else
				// {
				// 	prev_subtitle_text = strdup(subtitle_text);
				// }

				if (!prev_subtitle_text && subtitle_text)
				{
					prev_begin_time = prev_end_time + 1;
					prev_end_time = convert_pts_to_ms(ctx->packet.pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
					prev_subtitle_text = strdup(subtitle_text);
					prev_sub_encoded = 0;
				}
				prev_packet_pts = ctx->packet.pts;
			}
		}
		av_packet_unref(&ctx->packet);
	}

	if (!prev_sub_encoded)
	{
		add_cc_sub_text(ctx->dec_sub, prev_subtitle_text, prev_begin_time, prev_end_time, "", "BURN", CCX_ENC_UTF_8);
		encode_sub(enc_ctx, ctx->dec_sub);
		prev_sub_encoded = 1;
	}
	activity_progress(100, cur_sec / 60, cur_sec % 60);
}

void hardsubx_process_frames_binary(struct lib_hardsubx_ctx *ctx)
{
	// Do a binary search over the input video for faster processing
	// printf("Duration: %d\n", (int)ctx->format_ctx->duration);
	int got_frame;
	int seconds_time = 0;
	for (seconds_time = 0; seconds_time < 20; seconds_time++)
	{
		int64_t seek_time = seconds_time * AV_TIME_BASE;
		seek_time = av_rescale_q(seek_time, AV_TIME_BASE_Q, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);

		int ret = av_seek_frame(ctx->format_ctx, ctx->video_stream_id, seek_time, AVSEEK_FLAG_BACKWARD);
		// printf("%d\n", ret);
		// if(ret < 0)
		// {
		// 	printf("seeking back\n");
		// 	ret = av_seek_frame(ctx->format_ctx, -1, seek_time, AVSEEK_FLAG_BACKWARD);
		// }
		if (ret >= 0)
		{
			while (av_read_frame(ctx->format_ctx, &ctx->packet) >= 0)
			{
				if (ctx->packet.stream_index == ctx->video_stream_id)
				{
					avcodec_decode_video2(ctx->codec_ctx, ctx->frame, &got_frame, &ctx->packet);
					if (got_frame)
					{
						// printf("%d\n", seek_time);
						if (ctx->packet.pts < seek_time)
							continue;
						// printf("GOT FRAME: %d\n",ctx->packet.pts);
						// sws_scale is used to convert the pixel format to RGB24 from all other cases
						sws_scale(
						    ctx->sws_ctx,
						    (uint8_t const *const *)ctx->frame->data,
						    ctx->frame->linesize,
						    0,
						    ctx->codec_ctx->height,
						    ctx->rgb_frame->data,
						    ctx->rgb_frame->linesize);
						// Send the frame to other functions for processing
						_display_frame(ctx, ctx->rgb_frame, ctx->codec_ctx->width, ctx->codec_ctx->height, seconds_time);
						break;
					}
				}
				av_packet_unref(&ctx->packet);
			}
		}
		else
		{
			printf("Seeking to timestamp failed\n");
		}
	}
}

#endif
