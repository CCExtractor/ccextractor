#include "lib_ccx.h"
#include "utility.h"

#ifdef ENABLE_HARDSUBX
//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include "allheaders.h"
#include "hardsubx.h"
#include "capi.h"

//Global Flags
int xflag = 0;
int last_index =0;
int skip_frames = 12;

int continue_flag = 0;
int count_frame = 0;
int pre_index;
int pre_flag;
int t_flag = 0;

//Global Offset for Stitching
int offset = 5;

PIX *pix_temp, pix_temp_a;
PIX *pixa, *pix1, *second_pix;
//For Width and Height of the ticker
l_int32      w, d;

//For Late Fusion
char *temptext = "";
int xcounter = 0;


char *get_ocr_text_letterwise_russian(struct lib_hardsubx_ctx *ctx, PIX *image)
{
	char *text_out=NULL;

	if(TessBaseAPIInit3(ctx->tess_handle, NULL, "rus") != 0)
		die("Russian Language Not Set for Tesseract\n");

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if(TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{	
		mprint("Error in Tesseract recognition, skipping symbol\n");
		return NULL;
	}

	TessResultIterator *it = TessBaseAPIGetIterator(ctx->tess_handle);
	TessPageIteratorLevel level = RIL_SYMBOL;

	if(it!=0)
	{
		do
		{
			char* letter;
			char* ts_letter = TessResultIteratorGetUTF8Text(it, level);
			if(ts_letter==NULL || strlen(ts_letter)==0)
				continue;
			letter = strdup(ts_letter);
			TessDeleteText(ts_letter);
			if(text_out==NULL)
			{
				text_out = strdup(letter);
				continue;
			}
			text_out = realloc(text_out, strlen(text_out) + strlen(letter) + 1);
			strcat(text_out, letter);
			free(letter);
		} while(TessPageIteratorNext((TessPageIterator *)it, level));
	}

	TessResultIteratorDelete(it);

	return text_out;
}

//Simple Russian OCR with Tesseract
char* russian_ocr(PIX* img, int index){
	TessBaseAPI *handle;
	char *text;
        FILE * fp; 
        char write_path[100];

	handle = TessBaseAPICreate();

	if(TessBaseAPIInit3(handle, NULL, "rus") != 0)
		die("Russian Language Not Set for Tesseract\n");

	TessBaseAPISetImage2(handle, img);

	if(TessBaseAPIRecognize(handle, NULL) != 0)
		die("Error in Tesseract recognition\n");

	if((text = TessBaseAPIGetUTF8Text(handle)) == NULL)
		die("Error getting text\n");

        //Writing to a File
        sprintf(write_path,"txt%04d.txt",index);
        fp = fopen(write_path, "w");
        fprintf(fp, "%s", text);
        fclose(fp);

        //Displaying it
	fputs(text, stdout);


	//TessDeleteText(text);
	TessBaseAPIEnd(handle);
	TessBaseAPIDelete(handle);
        return text;

}


char *get_ocr_text_russian_simple_threshold(struct lib_hardsubx_ctx *ctx, PIX *image, float threshold)
{
	char *text_out=NULL;

	if(TessBaseAPIInit3(ctx->tess_handle, NULL, "rus") != 0)
		die("Russian Language Not Set for Tesseract\n");

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if(TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{	
		mprint("Error in Tesseract recognition, skipping frame\n");
		return NULL;
	}

	if((text_out = TessBaseAPIGetUTF8Text(ctx->tess_handle)) == NULL)
	{
		mprint("Error getting text, skipping frame\n");
	}

	int conf = TessBaseAPIMeanTextConf(ctx->tess_handle);

	if(conf < threshold)
		return NULL;

	ctx->cur_conf = (float)conf;

	return text_out;
}


//Writing File for Late Fusion
void writetofile(char* text, int index){
        FILE * fp; 
        char write_path[100];
        //Writing to a File
        sprintf(write_path,"txt%04d.txt",index);
        fp = fopen(write_path, "w");
        fprintf(fp, "%s", text);
        fclose(fp);

	fputs(text, stdout);

}

//Searching Words in Late Fusion
int searchword(char *text, char *word) {
    int i;

    while (*text != '\0') {
        while (isspace((unsigned char) *text))
            text++;
        for (i = 0; *text == word[i] && *text != '\0'; text++, i++);
        if ((isspace((unsigned char) *text) || *text == '\0') && word[i] == '\0')
           return 1;
        while (!isspace((unsigned char) *text) && *text != '\0')
            text++;
    }

    return 0;
}

//Late Fusion Combination
char* late_fusion(PIX* img, int index){

  char *text;
  char *writetext;

  char write_path[500];
  text = russian_ocr(img,index);
  writetext = russian_ocr(img,index);
  printf("%s", text);
  if(xcounter == 0){
  temptext = text;
  xcounter++;
  }

  //printf("%s", temptext);
  int totalmatch = 0;
  char *word = strtok(text, " ");
  //printf("%s", word);

   while (word != NULL) {
    if (searchword(word, temptext)) {
         printf("Match: %s\n", word);
         sprintf(write_path,"im%04d.jpg",index);
         pixWrite(write_path,img,IFF_JFIF_JPEG);
         writetofile(writetext, index);
         return writetext;
    }
    else{
         temptext = text;
    }
    word = strtok(NULL, " ");
   }  
}

//Late Fusion Process
char* _process_frame_tickertext_russian_latefusion(struct lib_hardsubx_ctx *ctx, AVFrame *frame, int width, int height, int index){
      printf("index%d\n", index);    
	PIX *im;
	PIX *edge_im;
	PIX *lum_im;
	PIX *feat_im;
	char *subtitle_text=NULL;
	im = pixCreate(width,height,32);
	lum_im = pixCreate(width,height,32);
	feat_im = pixCreate(width,height,32);
	int i,j;



	for(i=(92*height)/100;i<height;i++)
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
			if(L > 85)
				pixSetRGBPixel(lum_im,j,i,255,255,255);
			else
				pixSetRGBPixel(lum_im,j,i,0,0,0);
		}
	}



	//Handle the edge image
	edge_im = pixCreate(width,height,8);
	edge_im = pixConvertRGBToGray(im,0.0,0.0,0.0);
	edge_im = pixSobelEdgeFilter(edge_im, L_VERTICAL_EDGES);
	edge_im = pixDilateGray(edge_im, 21, 11);
	edge_im = pixThresholdToBinary(edge_im,45);

	for(i=92*(height/100);i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			unsigned int p1,p2,p3;
			pixGetPixel(edge_im,j,i,&p1);
			pixGetPixel(lum_im,j,i,&p3);
			if(p1==0&&p3>0)
				pixSetRGBPixel(feat_im,j,i,255,255,255);
			else
				pixSetRGBPixel(feat_im,j,i,0,0,0);
		}
	}

       char write_path[100];
       int num_of_occr = 0;
       int stop = 0;
       char *text;
       for(i=(92*height)/100;i<height && !stop;i++){
            for(j=0; j<width  && !stop; j++){
 
                 int p=j*3+i*frame->linesize[0];
			int r=frame->data[0][p];
			int g=frame->data[0][p+1];
			int b=frame->data[0][p+2];


         BOX* box = boxCreate(100, 525, 615, 25);
         PIX* pixd= pixClipRectangle(feat_im, box, NULL);
         last_index = index;
         subtitle_text = late_fusion(pixd, index);
         //writetofile(russian_ocr(pixd, index), index);
         boxDestroy(&box);
	 pixDestroy(&pixd);
         stop = 1;
     }
}

	pixDestroy(&lum_im);
	pixDestroy(&im);
	pixDestroy(&edge_im);
	pixDestroy(&feat_im);

        return subtitle_text;

}

//Detecting Color

int detect_stopper_color(int r, int g, int b,struct lib_hardsubx_ctx *ctx){

//For Detecting Blue Color
//Upper and Lower Limit
  if (r>=ctx->lower_red && r<=ctx->upper_red){
            if(g >=ctx->lower_green && g<=ctx->upper_green){
              if(b>=ctx->lower_blue && b<=ctx->upper_blue){
                  return 1;
       }
}
}
   else return 0;
}

void die(const char *errstr) {
	fputs(errstr, stderr);
	exit(1);
}

float check_frames(PIX* im1, PIX* im2){
  l_int32  type, comptype, d1, d2, same, first, last;
  l_float32    fract, diff, rmsdiff;
  NUMA *na1;
  PIX *pixd, *pix1;
       char write_path[100];

  d1 = pixGetDepth(im1);
  d2 = pixGetDepth(im2);

  if (d1 == 1 && d2 == 1) {
   pixEqual(im1, im2, &same);
        if (same) {
            return 1;
        }
        else {
                comptype = L_COMPARE_XOR;
                pixCompareBinary(im1, im2, comptype, &fract, &pixd);
                return fract;
        }
  }

  else {
           //comptype = L_COMPARE_ABS_DIFF;
           comptype = L_COMPARE_SUBTRACT;
           pixCompareGrayOrRGB(im1, im2, comptype, GPLOT_PNG, &same, &diff, &rmsdiff, &pixd);
            if (same){
                 return 1;
            }

            else {
                fprintf(stderr, "Images differ: <diff> = %10.6f\n", diff);
                return diff;
            }
       }
  }


char* stitch_images(PIX* im1, PIX* im2, int index, int endpoint){
  l_int32  type, comptype, d1, d2, same, first, last;
  l_float32    fract, diff, rmsdiff;

  l_int32 a_same;
  NUMA *na1;
  PIX *pixd, *pix1;
  char write_path[100];	
  char *subtitle_text=NULL;

  d1 = pixGetDepth(im1);
  d2 = pixGetDepth(im2);

  //printf("%s","MMuna");
  pixEqual(im1, second_pix, &a_same);


  if (d1 == 1 && d2 == 1) {
   pixEqual(im1, im2, &same);
        if (same) {
            fprintf(stderr, "Images are identical\n");
            pixd = pixCreateTemplate(im1);  /* write empty pix for diff */
        }
        else {
                comptype = L_COMPARE_XOR;
                pixCompareBinary(im1, im2, comptype, &fract, &pixd);
        }
  }

  else {
           //comptype = L_COMPARE_ABS_DIFF;
           comptype = L_COMPARE_SUBTRACT;
           pixCompareGrayOrRGB(im1, im2, comptype, GPLOT_PNG, &same, &diff, &rmsdiff, &pixd);
            if (same){
                fprintf(stderr, "Images are identical\n");
            }

            else {
                //fprintf(stderr, "Images differ: <diff> = %10.6f\n", diff);
                //fprintf(stderr, "               <rmsdiff> = %10.6f\n", rmsdiff);

                 if (diff >= 20){
                    pixSaveTiled(im1, pixa, 1.0, 0, 1,1);
                    pixSaveTiled(im2, pixa, 1.0, 0, 1,1);
                    pix1 = pixaDisplay(pixa, w*2, 0.80*d);
                    sprintf(write_path,"im%04d.jpg",index);
                    if (count_frame % 2 == 0){
                    pixWrite(write_path,pix1,IFF_JFIF_JPEG);
                    subtitle_text = russian_ocr(pix1,index);
                    }
                    count_frame++;
                    pixa = pixaCreate(5);
                    
                }
            }

       if (d1 != 16 && !same) {

            na1 = pixCompareRankDifference(im1, im2, 1);
            if (na1) {

              if (na1->array[250] < 10){
               
              printf("%20.10f\n", na1->array[250]);
               }

             }
         }
    }

return subtitle_text;

}

char* _process_frame_tickertext_russian(struct lib_hardsubx_ctx *ctx, AVFrame *frame, int width, int height, int index)
{
	PIX *im;
	PIX *edge_im;
	PIX *lum_im;
	PIX *feat_im;
	char *subtitle_text=NULL;
	im = pixCreate(width,height,32);
	lum_im = pixCreate(width,height,32);
	feat_im = pixCreate(width,height,32);
	int i,j;
        //printf("Governmenty Muna and Tuna and Chunna for the Luna and Tuna: %d", t_flag);
	for(i=(92*height)/100;i<height;i++)
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
			if(L > 85)
				pixSetRGBPixel(lum_im,j,i,255,255,255);
			else
				pixSetRGBPixel(lum_im,j,i,0,0,0);
		}
	}


	//Handle the edge image
	edge_im = pixCreate(width,height,8);
	edge_im = pixConvertRGBToGray(im,0.0,0.0,0.0);
	edge_im = pixSobelEdgeFilter(edge_im, L_VERTICAL_EDGES);
	edge_im = pixDilateGray(edge_im, 21, 11);
	edge_im = pixThresholdToBinary(edge_im,50);

	for(i=92*(height/100);i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			unsigned int p1,p2,p3;
			pixGetPixel(edge_im,j,i,&p1);
			// pixGetPixel(pixd,j,i,&p2);
			pixGetPixel(lum_im,j,i,&p3);
			if(p1==0&&p3>0)
				pixSetRGBPixel(feat_im,j,i,255,255,255);
			else
				pixSetRGBPixel(feat_im,j,i,0,0,0);
		}
	}





       int num_of_occr = 0;
       int stop = 0;
       char write_path[500];
       for(i=(92*height)/100;i<height && !stop;i++){
            for(j=0; j<width  && !stop; j++){
 
                 int p=j*3+i*frame->linesize[0];
			int r=frame->data[0][p];
			int g=frame->data[0][p+1];
			int b=frame->data[0][p+2];


        if (detect_stopper_color(r,g,b,ctx) == 1 && (index == ctx->start_frame ||index % ctx->frame_skip == 0) && t_flag == 0){
         //printf("%s\n", "Stopper Detected");  
         xflag = 1;
         t_flag = 1;
         continue_flag = 0;
         //Hardcoded Bouding Box Locations
         BOX* box = boxCreate(100, 525, 615, 25);
         PIX* pixd= pixClipRectangle(feat_im, box, NULL);
         last_index = index;
         pix_temp = pixCopy(NULL, pixd);
         subtitle_text = russian_ocr(pixd, index);
         pixGetDimensions(pixd, &w, NULL, &d);
         pre_index = index;
         //sprintf(write_path,"im%04d.jpg",index);
         //pixWrite(write_path,pixd,IFF_JFIF_JPEG);
         boxDestroy(&box);
	 pixDestroy(&pixd);
         stop = 1;
         }

         else if (detect_stopper_color(r,g,b,ctx) == 1 && (index > (pre_index + 15)) && t_flag == 1){
         //printf("%s\n", "Stopper Detected");  

         if (continue_flag == 0){
         t_flag = 0;
         printf("%s", "Here");
         //Hardcoded Bouding Box Locations
         BOX* box = boxCreate(100, 525, 615, 25);
         PIX* pixd= pixClipRectangle(feat_im, box, NULL);
         last_index = index;
         //subtitle_text = russian_ocr(pixd, index);
         printf("%f", check_frames(pixd, pix_temp));

         if (check_frames(pixd, pix_temp) <= 30){
         t_flag = 0;
         sprintf(write_path,"im%04d.jpg",pre_index);
         if (count_frame % 2 == 0) {
         pixWrite(write_path,pix_temp,IFF_JFIF_JPEG);
         subtitle_text = russian_ocr(pixd, index);
              }
         count_frame++;
         }
 
         else{
         t_flag = 0;
         sprintf(write_path,"im%04d.jpg",pre_index);
         if (count_frame % 2 == 0) {
         pixWrite(write_path,pix_temp,IFF_JFIF_JPEG);
         sprintf(write_path,"im%04d.jpg",index);
         pixWrite(write_path,pixd,IFF_JFIF_JPEG);
         subtitle_text = russian_ocr(pixd, index);
         }
         count_frame++;
         pix_temp = pixCopy(NULL, pixd);
         boxDestroy(&box);
	 pixDestroy(&pixd);
         continue_flag = 0;
               }
         }
     
         else if (continue_flag == 1){
         t_flag = 0;
         BOX* box = boxCreate(100, 525, 615, 25);
         PIX* pixd= pixClipRectangle(feat_im, box, NULL);
         last_index = index;
         subtitle_text = stitch_images(pix_temp, pixd, index,1);
         //subtitle_text = russian_ocr(pixd, index);
         boxDestroy(&box);
	 pixDestroy(&pixd);
         stop = 1;
         continue_flag = 0;
         }

       }
         //Checking of text after blue stopper
         else if(detect_stopper_color(r,g,b,ctx) == 0 && index % (ctx->frame_skip) == 0 && xflag == 1){
         continue_flag = 1;
         BOX* box = boxCreate(100, 525, 615, 25);
         PIX* pixd= pixClipRectangle(feat_im, box, NULL);
         //subtitle_text = russian_ocr(pixd, index);
         subtitle_text = stitch_images(pix_temp, pixd, index,0);
         //subtitle_text = russian_ocr(pixd, index);
         pre_index = index;
         pix_temp = pixCopy(NULL, pixd);
         //pixSaveTiled(pixd, pixa, 1.0, 0, 1,1);
         //sprintf(write_path,"im%04d.jpg",index);
         //pixWrite(write_path,pixd,IFF_JFIF_JPEG);
         boxDestroy(&box);
	 pixDestroy(&pixd);
         stop = 1;
        }
     }
}

	// Tesseract OCR for the ticker text here

	pixDestroy(&lum_im);
	pixDestroy(&im);
	pixDestroy(&edge_im);
	pixDestroy(&feat_im);

	return subtitle_text;
}

int hardsubx_process_frames_tickertext_russian_latefusion(struct lib_hardsubx_ctx *ctx, struct encoder_ctx *enc_ctx)
{
	// Search for ticker text at the bottom of the screen, such as in Russia TV1 or stock prices
	int got_frame;
	int cur_sec,total_sec,progress;
	int frame_number = 0;
	char *ticker_text = NULL;

	while(av_read_frame(ctx->format_ctx, &ctx->packet)>=0)
	{
		if(ctx->packet.stream_index == ctx->video_stream_id)
		{
			frame_number++;
			//Decode the video stream packet
			avcodec_decode_video2(ctx->codec_ctx, ctx->frame, &got_frame, &ctx->packet);
			if(got_frame && (frame_number == ctx->start_frame || frame_number % ctx->frame_skip == 0))
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

				ticker_text = _process_frame_tickertext_russian_latefusion(ctx,ctx->rgb_frame,ctx->codec_ctx->width,ctx->codec_ctx->height,frame_number);
				//printf("frame_number: %d\n", frame_number);

				if(ticker_text != NULL)printf("%s\n", ticker_text);

				cur_sec = (int)convert_pts_to_s(ctx->packet.pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
				total_sec = (int)convert_pts_to_s(ctx->format_ctx->duration, AV_TIME_BASE_Q);
				progress = (cur_sec*100)/total_sec;
				activity_progress(progress,cur_sec/60,cur_sec%60);
			}
		}
		av_packet_unref(&ctx->packet);
	}
	activity_progress(100,cur_sec/60,cur_sec%60);
	return 0;
}




int hardsubx_process_frames_tickertext_russian(struct lib_hardsubx_ctx *ctx, struct encoder_ctx *enc_ctx)
{
	// Search for ticker text at the bottom of the screen, such as in Russia TV1 or stock prices
	int got_frame;
	int cur_sec,total_sec,progress;
	int frame_number = 0;
	char *ticker_text = NULL;

	while(av_read_frame(ctx->format_ctx, &ctx->packet)>=0)
	{
		if(ctx->packet.stream_index == ctx->video_stream_id)
		{
			frame_number++;
			//Decode the video stream packet
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

				ticker_text = _process_frame_tickertext_russian(ctx,ctx->rgb_frame,ctx->codec_ctx->width,ctx->codec_ctx->height,frame_number);
				//printf("frame_number: %d\n", frame_number);

				if(ticker_text != NULL)printf("%s\n", ticker_text);

				cur_sec = (int)convert_pts_to_s(ctx->packet.pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
				total_sec = (int)convert_pts_to_s(ctx->format_ctx->duration, AV_TIME_BASE_Q);
				progress = (cur_sec*100)/total_sec;
				activity_progress(progress,cur_sec/60,cur_sec%60);
			}
		}
		av_packet_unref(&ctx->packet);
	}
	activity_progress(100,cur_sec/60,cur_sec%60);
	return 0;
}


char* _process_frame_white_basic_russian(struct lib_hardsubx_ctx *ctx, AVFrame *frame, int width, int height, int index)
{
	//printf("frame : %04d\n", index);
	PIX *im;
	PIX *edge_im;
	PIX *lum_im;
	PIX *feat_im;
	char *subtitle_text=NULL;
	im = pixCreate(width,height,32);
	lum_im = pixCreate(width,height,32);
	feat_im = pixCreate(width,height,32);
	int i,j;
        char write_path[500];
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
			if(L > 85)
				pixSetRGBPixel(lum_im,j,i,255,255,255);
			else
				pixSetRGBPixel(lum_im,j,i,0,0,0);
		}
	}

	//Handle the edge image
	edge_im = pixCreate(width,height,8);
	edge_im = pixConvertRGBToGray(im,0.0,0.0,0.0);
	edge_im = pixSobelEdgeFilter(edge_im, L_VERTICAL_EDGES);
	edge_im = pixDilateGray(edge_im, 21, 11);
	edge_im = pixThresholdToBinary(edge_im,50);

	for(i=3*(height/4);i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			unsigned int p1,p2,p3;
			pixGetPixel(edge_im,j,i,&p1);
			// pixGetPixel(pixd,j,i,&p2);
			pixGetPixel(lum_im,j,i,&p3);
			if(p1==0&&p3>0)
				pixSetRGBPixel(feat_im,j,i,255,255,255);
			else
				pixSetRGBPixel(feat_im,j,i,0,0,0);
		}
	}

	if(ctx->conf_thresh > 0)
              subtitle_text = get_ocr_text_russian_simple_threshold(ctx,feat_im, 45);
        else if(ctx->letter_russian)
              subtitle_text = get_ocr_text_letterwise_russian(ctx,feat_im);
              //subtitle_text = russian_ocr(feat_im,index);
        else
              subtitle_text = russian_ocr(feat_im,index);
              //subtitle_text = get_ocr_text_letterwise_russian(ctx,feat_im);
        //sprintf(write_path,"im%04d.jpg",index);
        //pixWrite(write_path,feat_im,IFF_JFIF_JPEG);
	

	pixDestroy(&lum_im);
	pixDestroy(&im);
	pixDestroy(&edge_im);
	pixDestroy(&feat_im);

	return subtitle_text;
}


char *_process_frame_color_basic_russian(struct lib_hardsubx_ctx *ctx, AVFrame *frame, int width, int height, int index)
{
	char *subtitle_text=NULL;
	PIX *im;
	im = pixCreate(width,height,32);
	PIX *hue_im = pixCreate(width,height,32);

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
			float H,S,V;
			rgb_to_hsv((float)r,(float)g,(float)b,&H,&S,&V);
			if(abs(H-ctx->hue)<20)
			{
				pixSetRGBPixel(hue_im,j,i,r,g,b);
			}
		}
	}

	PIX *edge_im = pixCreate(width,height,8),*edge_im_2 = pixCreate(width,height,8);
	edge_im = pixConvertRGBToGray(im,0.0,0.0,0.0);
	edge_im = pixSobelEdgeFilter(edge_im, L_VERTICAL_EDGES);
	edge_im = pixDilateGray(edge_im, 21, 1);
	edge_im = pixThresholdToBinary(edge_im,50);
	PIX *pixd = pixCreate(width,height,1);
	pixSauvolaBinarize(pixConvertRGBToGray(hue_im,0.0,0.0,0.0), 15, 0.3, 1, NULL, NULL, NULL, &pixd);

	edge_im_2 = pixConvertRGBToGray(hue_im,0.0,0.0,0.0);
	edge_im_2 = pixDilateGray(edge_im_2, 5, 5);

	PIX *feat_im = pixCreate(width,height,32);
	for(i=3*(height/4);i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			unsigned int p1,p2,p3,p4;
			pixGetPixel(edge_im,j,i,&p1);
			pixGetPixel(pixd,j,i,&p2);
			// pixGetPixel(hue_im,j,i,&p3);
			pixGetPixel(edge_im_2,j,i,&p4);
			if(p1==0&&p2==0&&p4>0)//if(p4>0&&p1==0)//if(p2==0&&p1==0&&p3>0)
			{
				pixSetRGBPixel(feat_im,j,i,255,255,255);
			}
		}
	}


        subtitle_text = russian_ocr(feat_im,index);


	pixDestroy(&feat_im);
	pixDestroy(&im);
	pixDestroy(&edge_im);
	pixDestroy(&hue_im);
	pixDestroy(&edge_im_2);
	pixDestroy(&pixd);

	return subtitle_text;
}

int hardsubx_process_frames_linear_russian(struct lib_hardsubx_ctx *ctx, struct encoder_ctx *enc_ctx)
{
	// Do an exhaustive linear search over the video

	int prev_sub_encoded = 1; // Previous seen subtitle encoded or not
	int got_frame;
	int dist = 0;
	int cur_sec,total_sec,progress;
	int frame_number = 0;
	int64_t prev_begin_time = 0, prev_end_time = 0; // Begin and end time of previous seen subtitle
	int64_t prev_packet_pts = 0;
	char *subtitle_text=NULL; // Subtitle text of current frame
	char *prev_subtitle_text=NULL; // Previously seen subtitle text

	while(av_read_frame(ctx->format_ctx, &ctx->packet)>=0)
	{
		if(ctx->packet.stream_index == ctx->video_stream_id)
		{
			frame_number++;

			//Decode the video stream packet
			avcodec_decode_video2(ctx->codec_ctx, ctx->frame, &got_frame, &ctx->packet);

			if(got_frame && (frame_number == ctx->start_frame || frame_number % ctx->frame_skip == 0))
			{
				float diff = (float)convert_pts_to_ms(ctx->packet.pts - prev_packet_pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
				if(abs(diff) < 1000*ctx->min_sub_duration) //If the minimum duration of a subtitle line is exceeded, process packet
					continue;

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
				if(ctx->subcolor==HARDSUBX_COLOR_WHITE)
				{
	                         subtitle_text = _process_frame_white_basic_russian(ctx,ctx->rgb_frame,ctx->codec_ctx->width,ctx->codec_ctx->height,frame_number); 
                                }
				else{
				 subtitle_text = _process_frame_color_basic_russian(ctx,ctx->rgb_frame,ctx->codec_ctx->width,ctx->codec_ctx->height,frame_number);
 				}
				_display_frame(ctx, ctx->rgb_frame,ctx->codec_ctx->width,ctx->codec_ctx->height,frame_number);

				cur_sec = (int)convert_pts_to_s(ctx->packet.pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
				total_sec = (int)convert_pts_to_s(ctx->format_ctx->duration, AV_TIME_BASE_Q);
				progress = (cur_sec*100)/total_sec;
				activity_progress(progress,cur_sec/60,cur_sec%60);

				if((!subtitle_text && !prev_subtitle_text) || (subtitle_text && !strlen(subtitle_text) && !prev_subtitle_text)) {
					prev_end_time = convert_pts_to_ms(ctx->packet.pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
				}

				if(subtitle_text) {
					char *double_enter = strstr(subtitle_text,"\n\n");
					if(double_enter!=NULL)
						*(double_enter)='\0';
				}

				if(!prev_sub_encoded && prev_subtitle_text)
				{
					if(subtitle_text)
					{
						dist = edit_distance(subtitle_text, prev_subtitle_text, strlen(subtitle_text), strlen(prev_subtitle_text));
						if(dist < (0.2 * fmin(strlen(subtitle_text), strlen(prev_subtitle_text))))
						{
							dist = -1;
							subtitle_text = NULL;
							prev_end_time = convert_pts_to_ms(ctx->packet.pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
						}
					}
					if(dist != -1)
					{
						add_cc_sub_text(ctx->dec_sub, prev_subtitle_text, prev_begin_time, prev_end_time, "", "BURN", CCX_ENC_UTF_8);
						encode_sub(enc_ctx, ctx->dec_sub);
						prev_begin_time = prev_end_time + 1;
						prev_subtitle_text = NULL;
						prev_sub_encoded = 1;
						prev_end_time = convert_pts_to_ms(ctx->packet.pts, ctx->format_ctx->streams[ctx->video_stream_id]->time_base);
						if(subtitle_text) {
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

				if(!prev_subtitle_text && subtitle_text) {
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

	if(!prev_sub_encoded) {
		add_cc_sub_text(ctx->dec_sub, prev_subtitle_text, prev_begin_time, prev_end_time, "", "BURN", CCX_ENC_UTF_8);
		encode_sub(enc_ctx, ctx->dec_sub);
		prev_sub_encoded = 1;
	}
	activity_progress(100,cur_sec/60,cur_sec%60);

}

#endif


