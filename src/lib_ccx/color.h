#ifndef COLOR_H
#define COLOR_H

#include "Mat.h"

#define CV_DESCALE(x,n)     (((x) + (1 << ((n)-1))) >> (n))

typedef struct CvtHelper
{
    Mat src, dst;
    int depth, scn;
    Size dstSz;
} CvtHelper ;

typedef struct CvtColorLoop_Invoker
{
    const unsigned char* src_data;
    const size_t src_step;
    unsigned char* dst_data;
    const size_t dst_step;
    const int width;
    const void* cvt;
} CvtColorLoop_Invoker ;

typedef struct RGB2HLS_f
{
    int srccn, blueIdx;
    float hscale;
} RGB2HLS_f ;

typedef struct RGB2HLS_b
{
    int srccn;
    RGB2HLS_f cvt;
} RGB2HLS_b ;

typedef struct RGB2Lab_b
{
    int srccn;
    int coeffs[9];
    bool srgb;
} RGB2Lab_b ;

typedef struct RGBtoGray
{
    int srccn;
    int tab[256*3];
} RGBtoGray ;

CvtHelper cvtHelper(Mat _src, Mat* _dst, int dcn)
{
    CvtHelper h;
    int stype = type(src);
    h.scn = CV_MAT_CN(stype), h.depth = CV_MAT_DEPTH(stype);

    h.src = _src;

    Size sz = init_size(h.src.cols, h.src.rows);
    h.dstSz = sz;
    create(_dst, h.src.rows, h.src.cols, CV_MAKETYPE(h.depth, dcn));
    h.dst = *_dst;
    return h;
}

CvtColorLoop_Invoker cvtColorLoop_Invoker(const unsigned char* src_data_, size_t src_step_, unsigned char* dst_data_, size_t dst_step_, int width_, const void* _cvt, int code_cvt)
{
    CvtColorLoop_Invoker cli;
    cli.src_data = src_data_;
    cli.src_step = src_step_;
    cli.dst_data = dst_data_;
    cli.dst_step = dst_step_;
    cli.width = width_;
    cli.cvt = _cvt;
    return cli;
}

RGB2HLS_b RGB2HLSb(int _srccn, int _blueIdx, int _hrange)
{
    RGB2HLS_b rgb2hls;
    rgb2hls.srccn = _srccn;
    rgb2hls.cvt.srccn = 3;
    rgb2hls.cvt.blueIdx = _blueIdx;
    rgb2hls.cvt.hscale = (float)_hrange;
    return rgb2hls;
}

void RGB2Gray(RGBtoGray* r2g, int _srccn, int blueIdx, const int* coeffs)
{
    RGBtoGray rg;
    rg.srccn = _srccn;
    const int coeffs0[] = {R2Y, G2Y, B2Y};
    if(!coeffs) 
        coeffs = coeffs0;

    int b = 0, g = 0, r = (1 << 13);
    int db = coeffs[blueIdx^2], dg = coeffs[1], dr = coeffs[blueIdx];

    for(int i = 0; i < 256; i++, b += db, g += dg, r += dr)
    {
        rg.tab[i] = b;
        rg.tab[i+256] = g;
        rg.tab[i+512] = r;
    }
    return rg;
}

void RGB2gray(RGBtoGray* cvt, unsigned char* src, unsigned char* dst, int n)
{
    int scn = cvt->srccn;
    const int* _tab = cvt->tab;
    for(int i = 0; i < n; i++, src += scn)
        dst[i] = (unsigned char)((_tab[src[0]] + _tab[src[1]+256] + _tab[src[2]+512]) >> 14);
}

void RGBtoHLS_f(RGB2HLS_f* body, const float* src, float* dst, int n)
{
    int i = 0, bidx = body->blueIdx, scn = body->srccn;
    n *= 3;

    for(; i < n; i += 3, src += scn)
    {
        float b = src[bidx], g = src[1], r = src[bidx^2];
        float h = 0.f, s = 0.f, l;
        float vmin, vmax, diff;

        vmax = vmin = r;
        if(vmax < g) vmax = g;
        if(vmax < b) vmax = b;
        if(vmin > g) vmin = g;
        if(vmin > b) vmin = b;

        diff = vmax - vmin;
        l = (vmax + vmin)*0.5f;

        if(diff > FLT_EPSILON)
        {
            s = l < 0.5f ? diff/(vmax + vmin) : diff/(2 - vmax - vmin);
            diff = 60.f/diff;

            if(vmax == r)
                h = (g-b)*diff;
            else if(vmax == g)
                h = (b-r)*diff + 120.f;
            else
                h = (r-g)*diff + 240.f;

            if(h < 0.f) h += 360.f;
        }

        dst[i] = h*hscale;
        dst[i+1] = l;
        dst[i+2] = s;
    }
}

void RGB2Hls_b(RGB2HLS_b* r2h, const unsigned char* src, unsigned char* dst, int n)
{
    int i, j, scn = r2h->srccn;
    float buf[3*BLOCK_SIZE];

    for(i = 0; i < n; i += BLOCK_SIZE, dst += BLOCK_SIZE*3)
    {
        int dn = min(n-i, (int)BLOCK_SIZE);
        j = 0;

        for(; j < dn*3; j += 3, src += scn)
        {
            buf[j] = src[0]*(1.f/255.f);
            buf[j+1] = src[1]*(1.f/255.f);
            buf[j+2] = src[2]*(1.f/255.f);
        }
        RGBtoHLS_f(&(r2h->cvt), buf, buf, dn);

        j = 0;
        for(; j < dn*3; j += 3)
        {
            dst[j] = (unsigned char)(buf[j]);
            dst[j+1] = (unsigned char)(buf[j+1]*255.f);
            dst[j+2] = (unsigned char)(buf[j+2]*255.f);
        }
    }
}

#endif