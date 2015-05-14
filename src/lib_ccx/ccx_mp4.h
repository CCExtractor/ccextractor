#ifndef CXX_MP4_H
#define CXX_MP4_H


struct ccx_s_mp4Cfg
{
	unsigned int mp4vidtrack :1;
};

int processmp4 (struct lib_ccx_ctx *ctx,struct ccx_s_mp4Cfg *cfg, char *file,void *enc_ctx);
#endif
