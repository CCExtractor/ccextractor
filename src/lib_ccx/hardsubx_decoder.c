#include "lib_ccx.h"
#include "utility.h"

#ifdef ENABLE_HARDSUBX
// TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <leptonica/allheaders.h>
#include <tesseract/capi.h>
#include "hardsubx.h"
#ifdef ENABLE_FFMPEG
#include "ffmpeg_intgr.h"
#endif

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
			if (p1 == 0 && p2 == 0 && p4 > 0) // if(p4>0&&p1==0)//if(p2==0&&p1==0&&p3>0)
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

int hardsubx_process_frames_tickertext(struct lib_hardsubx_ctx *ctx, struct encoder_ctx *enc_ctx)
{
	// Search for ticker text at the bottom of the screen, such as in Russia TV1 or stock prices
	int cur_sec = 0, total_sec, progress;
	int frame_number = 0;
	char *ticker_text = NULL;

	while (av_read_frame(ctx->format_ctx, &ctx->packet) >= 0)
	{
		if (ctx->packet.stream_index == ctx->video_stream_id)
		{
			frame_number++;
			// Decode the video stream packet
			avcodec_send_packet(ctx->codec_ctx, &ctx->packet);
			if (avcodec_receive_frame(ctx->codec_ctx, ctx->frame) == 0 && frame_number % 1000 == 0)
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

			// Decode the video stream packet
			avcodec_send_packet(ctx->codec_ctx, &ctx->packet);
			if (avcodec_receive_frame(ctx->codec_ctx, ctx->frame) == 0 && frame_number % 25 == 0)
			{
				float diff = (float)convert_pts_to_ms(ctx->packet.pts - prev_packet_pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
				if (fabsf(diff) < 1000 * ctx->min_sub_duration) // If the minimum duration of a subtitle line is exceeded, process packet
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
							free_rust_c_string(subtitle_text);
							subtitle_text = NULL;
							prev_end_time = convert_pts_to_ms(ctx->packet.pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
						}
					}
					if (dist != -1)
					{
						add_cc_sub_text(ctx->dec_sub, prev_subtitle_text, prev_begin_time, prev_end_time, "", "BURN", CCX_ENC_UTF_8);
						encode_sub(enc_ctx, ctx->dec_sub);
						prev_begin_time = prev_end_time + 1;
						free(prev_subtitle_text);
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

				// Free subtitle_text from this iteration (was allocated by Rust in _process_frame_*_basic)
				free_rust_c_string(subtitle_text);
				subtitle_text = NULL;
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

	// Cleanup
	free(prev_subtitle_text);
	activity_progress(100, cur_sec / 60, cur_sec % 60);
}

void process_hardsubx_linear_frames_and_normal_subs(struct lib_hardsubx_ctx *hard_ctx, struct encoder_ctx *enc_ctx, struct lib_ccx_ctx *ctx)
{
	// variables for cc extraction
	struct lib_cc_decode *dec_ctx = NULL;
	enum ccx_stream_mode_enum stream_mode;
	struct demuxer_data *datalist = NULL;
	struct demuxer_data *data_node = NULL;
	int (*get_more_data)(struct lib_ccx_ctx * c, struct demuxer_data * *d);
	int ret;
	int caps = 0;

	uint64_t min_pts = UINT64_MAX;

	// variables for burnt-in subtitle extraction
	int prev_sub_encoded_hard = 1; // Previous seen burnt-in subtitle encoded or not
	int dist = 0;
	int cur_sec = 0, total_sec, progress;
	int frame_number = 0;
	int64_t prev_begin_time_hard = 0, prev_end_time_hard = 0; // Begin and end time of previous seen burnt-in subtitle
	int64_t prev_packet_pts_hard = 0;
	char *subtitle_text_hard = NULL;      // Subtitle text of current frame (burnt_in)
	char *prev_subtitle_text_hard = NULL; // Previously seen burnt-in subtitle text

	stream_mode = ctx->demux_ctx->get_stream_mode(ctx->demux_ctx);

	if (stream_mode == CCX_SM_TRANSPORT && ctx->write_format == CCX_OF_NULL)
		ctx->multiprogram = 1;

	switch (stream_mode)
	{
		case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
			get_more_data = &general_get_more_data;
			break;
		case CCX_SM_TRANSPORT:
			get_more_data = &ts_get_more_data;
			break;
		case CCX_SM_PROGRAM:
			get_more_data = &ps_get_more_data;
			break;
		case CCX_SM_ASF:
			get_more_data = &asf_get_more_data;
			break;
		case CCX_SM_WTV:
			get_more_data = &wtv_get_more_data;
			break;
			// case CCX_SM_GXF:
			// 	get_more_data = &ccx_gxf_get_more_data;
			// 	break;
#ifdef ENABLE_FFMPEG
		case CCX_SM_FFMPEG:
			get_more_data = &ffmpeg_get_more_data;
			break;
#endif
		// case CCX_SM_MXF:
		// 	get_more_data = ccx_mxf_getmoredata;
		// 	// The MXFContext might have already been initialized if the
		// 	// stream type was autodetected
		// 	if (ctx->demux_ctx->private_data == NULL)
		// 	{
		// 		ctx->demux_ctx->private_data = ccx_gxf_init(ctx->demux_ctx);
		// 	}
		// 	break;
		default:
			fatal(CCX_COMMON_EXIT_BUG_BUG, "In general_loop: Impossible value for stream_mode");
	}
	end_of_file = 0;
	int status = 0;

	int last_cc_encoded = 0;

	while (true)
	{

		if (status < 0 && end_of_file == 1)
		{
			// status <= 0 implies either something is wrong or eof
			break;
		}
		if ((!terminate_asap && !end_of_file && is_decoder_processed_enough(ctx) == CCX_FALSE) && ret != CCX_EINVAL)
		{
			position_sanity_check(ctx->demux_ctx);
			ret = get_more_data(ctx, &datalist);
			if (ret == CCX_EOF)
			{
				end_of_file = 1;
			}

			if (datalist)
			{
				position_sanity_check(ctx->demux_ctx);

				if (!ctx->multiprogram)
				{

					if (dec_ctx == NULL || hard_ctx->dec_sub->start_time >= (dec_ctx->dec_sub).start_time || status < 0)
					{
						int ret = process_non_multiprogram_general_loop(ctx,
												&datalist,
												&data_node,
												&dec_ctx,
												&enc_ctx,
												&min_pts,
												ret,
												&caps);
					}
				}
				if (ctx->live_stream)
				{
					int cur_sec = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
					int th = cur_sec / 10;
					if (ctx->last_reported_progress != th)
					{
						activity_progress(-1, cur_sec / 60, cur_sec % 60);
						ctx->last_reported_progress = th;
					}
				}
				else
				{
					if (ctx->total_inputsize > 255) // Less than 255 leads to division by zero below.
					{
						int progress = (int)((((ctx->total_past + ctx->demux_ctx->past) >> 8) * 100) / (ctx->total_inputsize >> 8));
						if (ctx->last_reported_progress != progress)
						{
							LLONG t = get_fts(dec_ctx->timing, dec_ctx->current_field);
							if (!t && ctx->demux_ctx->global_timestamp_inited)
								t = ctx->demux_ctx->global_timestamp - ctx->demux_ctx->min_global_timestamp;
							int cur_sec = (int)(t / 1000);
							activity_progress(progress, cur_sec / 60, cur_sec % 60);
							ctx->last_reported_progress = progress;
						}
					}
				}

				// void segment_output_file(struct lib_ccx_ctx *ctx, struct lib_cc_decode *dec_ctx);
				segment_output_file(ctx, dec_ctx);

				if (ccx_options.send_to_srv)
					net_check_conn();
			}
		}
		if (end_of_file && !last_cc_encoded)
		{
			// the last closed caption needs to be encoded separately
			// if EOF file has been hit for cc
			// then encode one last time
			enc_ctx = update_encoder_list(ctx);
			flush_cc_decode(dec_ctx, &dec_ctx->dec_sub);
			encode_sub(enc_ctx, &dec_ctx->dec_sub);
			last_cc_encoded = 1;
		}
		if (hard_ctx->dec_sub->start_time <= (dec_ctx->dec_sub).start_time || end_of_file == 1)
		{
			status = av_read_frame(hard_ctx->format_ctx, &hard_ctx->packet);

			if (status >= 0 && hard_ctx->packet.stream_index == hard_ctx->video_stream_id)
			{
				frame_number++;

				avcodec_send_packet(hard_ctx->codec_ctx, &hard_ctx->packet);
				if (avcodec_receive_frame(hard_ctx->codec_ctx,
							  hard_ctx->frame) == 0 &&
				    frame_number % 25 == 0)
				{
					float diff = (float)convert_pts_to_ms(hard_ctx->packet.pts - prev_packet_pts_hard,
									      hard_ctx->format_ctx->streams[hard_ctx->video_stream_id]->time_base);

					if (fabsf(diff) >= 1000 * hard_ctx->min_sub_duration)
					{

						// sws_scale is used to convert the pixel format to RGB24 from all other cases
						sws_scale(
						    hard_ctx->sws_ctx,
						    (uint8_t const *const *)hard_ctx->frame->data,
						    hard_ctx->frame->linesize,
						    0,
						    hard_ctx->codec_ctx->height,
						    hard_ctx->rgb_frame->data,
						    hard_ctx->rgb_frame->linesize);

						if (hard_ctx->subcolor == HARDSUBX_COLOR_WHITE)
						{
							subtitle_text_hard = _process_frame_white_basic(hard_ctx,
													hard_ctx->rgb_frame,
													hard_ctx->codec_ctx->width,
													hard_ctx->codec_ctx->height,
													frame_number);
						}
						else
						{
							subtitle_text_hard = _process_frame_color_basic(hard_ctx,
													hard_ctx->rgb_frame,
													hard_ctx->codec_ctx->width,
													hard_ctx->codec_ctx->height,
													frame_number);
						}

						_display_frame(hard_ctx, hard_ctx->rgb_frame, hard_ctx->codec_ctx->width, hard_ctx->codec_ctx->height, frame_number);
						cur_sec = (int)convert_pts_to_s(hard_ctx->packet.pts, hard_ctx->format_ctx->streams[hard_ctx->video_stream_id]->time_base);
						total_sec = (int)convert_pts_to_s(hard_ctx->format_ctx->duration, AV_TIME_BASE_Q);
						progress = (cur_sec * 100) / total_sec;
						activity_progress(progress, cur_sec / 60, cur_sec % 60);
						// progress on burnt-in extraction
						if ((!subtitle_text_hard && !prev_subtitle_text_hard) || (subtitle_text_hard && !strlen(subtitle_text_hard) && !prev_subtitle_text_hard))
						{
							prev_end_time_hard = convert_pts_to_ms(hard_ctx->packet.pts, hard_ctx->format_ctx->streams[hard_ctx->video_stream_id]->time_base);
						}

						if (subtitle_text_hard)
						{
							char *double_enter = strstr(subtitle_text_hard, "\n\n");
							if (double_enter != NULL)
								*(double_enter) = '\0';
						}

						if (!prev_sub_encoded_hard && prev_subtitle_text_hard)
						{
							if (subtitle_text_hard)
							{
								dist = edit_distance(subtitle_text_hard, prev_subtitle_text_hard, (int)strlen(subtitle_text_hard), (int)strlen(prev_subtitle_text_hard));
								if (dist < (0.2 * MIN(strlen(subtitle_text_hard), strlen(prev_subtitle_text_hard))))
								{
									dist = -1;
									free_rust_c_string(subtitle_text_hard);
									subtitle_text_hard = NULL;
									prev_end_time_hard = convert_pts_to_ms(hard_ctx->packet.pts, hard_ctx->format_ctx->streams[hard_ctx->video_stream_id]->time_base);
								}
							}
							if (dist != -1)
							{
								add_cc_sub_text(hard_ctx->dec_sub, prev_subtitle_text_hard, prev_begin_time_hard, prev_end_time_hard, "", "BURN", CCX_ENC_UTF_8);
								encode_sub(enc_ctx, hard_ctx->dec_sub);
								prev_begin_time_hard = prev_end_time_hard + 1;
								free(prev_subtitle_text_hard);
								prev_subtitle_text_hard = NULL;
								prev_sub_encoded_hard = 1;
								prev_end_time_hard = convert_pts_to_ms(hard_ctx->packet.pts, hard_ctx->format_ctx->streams[hard_ctx->video_stream_id]->time_base);
								if (subtitle_text_hard)
								{
									prev_subtitle_text_hard = strdup(subtitle_text_hard);
									prev_sub_encoded_hard = 0;
								}
							}
							dist = 0;
						}

						if (!prev_subtitle_text_hard && subtitle_text_hard)
						{
							prev_begin_time_hard = prev_end_time_hard + 1;
							prev_end_time_hard = convert_pts_to_ms(hard_ctx->packet.pts, hard_ctx->format_ctx->streams[hard_ctx->video_stream_id]->time_base);
							prev_subtitle_text_hard = strdup(subtitle_text_hard);
							prev_sub_encoded_hard = 0;
						}
						prev_packet_pts_hard = hard_ctx->packet.pts;

						// Free subtitle_text_hard from this iteration (allocated by Rust)
						free_rust_c_string(subtitle_text_hard);
						subtitle_text_hard = NULL;
					}
				}
			}
			av_packet_unref(&hard_ctx->packet);
		}
	}
	if (!prev_sub_encoded_hard)
	{
		add_cc_sub_text(hard_ctx->dec_sub, prev_subtitle_text_hard, prev_begin_time_hard, prev_end_time_hard, "", "BURN", CCX_ENC_UTF_8);
		encode_sub(enc_ctx, hard_ctx->dec_sub);
		prev_sub_encoded_hard = 1;
	}

	// Cleanup
	free(prev_subtitle_text_hard);
	activity_progress(100, cur_sec / 60, cur_sec % 60);
}

void hardsubx_process_frames_binary(struct lib_hardsubx_ctx *ctx)
{
	// Do a binary search over the input video for faster processing
	// printf("Duration: %d\n", (int)ctx->format_ctx->duration);
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
					avcodec_send_packet(ctx->codec_ctx, &ctx->packet);
					if (avcodec_receive_frame(ctx->codec_ctx, ctx->frame) == 0)
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
