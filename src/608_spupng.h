#ifndef __608_SPUPNG_H__

#include "png.h"
#include "ccextractor.h"

struct spupng_t
{
    FILE* fpxml;
    FILE* fppng;
    char* dirname;
    char* pngfile;
    int fileIndex;
    int xOffset;
    int yOffset;
};

struct spupng_t *spunpg_init(struct s_context_cc608 *context);
void spunpg_free(struct spupng_t *spupng);

void spupng_write_header(struct spupng_t *spupng);
void spupng_write_footer(struct spupng_t *spupng);

int spupng_write_ccbuffer(struct spupng_t *spupng, struct eia608_screen* data,
                  struct s_context_cc608 *context);

void spupng_init_font();

int spupng_write_png(struct spupng_t *spupng,
             struct eia608_screen* data,
             png_structp png_ptr, png_infop info_ptr,
             png_bytep image,
             png_bytep* row_pointer,
             unsigned int ww, unsigned int wh);

int spupng_export_png(struct spupng_t *spupng, struct eia608_screen* data);

#endif /* __608_SPUPNG_H__ */
