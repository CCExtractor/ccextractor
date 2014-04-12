#ifndef __608_SPUPNG_H__

#include "png.h"
#include "ccextractor.h"

class SpuPng
{
    public:
        SpuPng(struct ccx_s_write* wb);
        ~SpuPng();

        void writeHeader();
        void writeFooter();

		int writeCCBuffer(struct eia608_screen* data, struct ccx_s_write *wb);

    private:
        static void initFont(void);
        int writePNG(struct eia608_screen* data,
            png_structp png_ptr, png_infop info_ptr,
            png_bytep image,
            png_bytep* row_pointer,
            unsigned int ww,
            unsigned int wh);
        int exportPNG(struct eia608_screen* data);

        FILE* fpxml;
        FILE* fppng;
        char* dirname;
        char* pngfile;
        int fileIndex;
        int xOffset;
        int yOffset;
};

#endif /* __608_SPUPNG_H__ */
