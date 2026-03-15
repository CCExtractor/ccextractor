#ifndef CXX_MP4_H
#define CXX_MP4_H

int processmp4(struct lib_ccx_ctx *ctx, struct ccx_s_mp4Cfg *cfg, char *file);
int dumpchapters(struct lib_ccx_ctx *ctx, struct ccx_s_mp4Cfg *cfg, char *file);
unsigned char *ccdp_find_data(unsigned char *ccdp_atom_content, unsigned int len, unsigned int *cc_count);

#ifdef ENABLE_FFMPEG_MP4
/* Rust FFmpeg MP4 demuxer entry points */
int ccxr_processmp4(struct lib_ccx_ctx *ctx, const char *file);
int ccxr_dumpchapters(struct lib_ccx_ctx *ctx, const char *file);
#endif

#endif
