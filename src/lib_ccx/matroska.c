#include "lib_ccx.h"
#include "file_buffer.h"
#include "ccx_common_platform.h"

FILE* create_file(struct lib_ccx_ctx *ctx)
{
    char* filename = ctx->inputfile[ctx->current_file];
    FILE* file = fopen(filename, "rb");
    return file;
}

int matroska_loop(struct lib_ccx_ctx *ctx)
{
    return 1;
}