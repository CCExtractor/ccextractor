#ifndef CXX_MP4_H
#define CXX_MP4_H

int processmp4(struct lib_ccx_ctx *ctx, struct ccx_s_mp4Cfg *cfg, char *file);
int dumpchapters(struct lib_ccx_ctx *ctx, struct ccx_s_mp4Cfg *cfg, char *file);

#ifdef ENABLE_FFMPEG_MP4

/* Rust-based MP4 demuxer - preferred when available */
int ccxr_processmp4(struct lib_ccx_ctx *ctx, char *file);
int ccxr_dumpchapters(struct lib_ccx_ctx *ctx, char *file);
#endif

#endif
