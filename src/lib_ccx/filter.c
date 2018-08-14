#include <stdlib.h>
#include <xmmintrin.h>

#include "filter.h"

void init_FilterEngine(FilterEngine* filter, BaseFilter* _filter2D, BaseDimFilter* _rowFilter, BaseDimFilter* _columnFilter,
                       int _srcType, int _dstType, int _bufType,
                       int _rowBorderType, int _columnBorderType, const Scalar _borderValue)
{
    filter = malloc(sizeof(FilterEngine));
    filter->srcType = filter->dstType = filter->bufType = -1;
    filter->maxWidth = filter->dx1 = filter->dx2 = 0;
    filter->wholeSize = init_Point(-1, -1);
    filter->rowBorderType = filter->columnBorderType = 1;
    filter->rows = malloc(sizeof(vector));
    vector_init(filter->rows);
    filter->borderElemSize = filter->bufStep = filter->startY = filter->startY0 = filter->rowCount = filter->dstY = 0;

    _srcType = CV_MAT_TYPE(_srcType);
    _bufType = CV_MAT_TYPE(_bufType);
    _dstType = CV_MAT_TYPE(_dstType);

    filter->srcType = _srcType;
    int srcElemSize = (int)CV_ELEM_SIZE(filter->srcType);
    filter->dstType = _dstType;
    filter->bufType = _bufType;

    filter->filter2D = _filter2D;
    filter->rowFilter = _rowFilter;
    filter->columnFilter = _columnFilter;

    if(_columnBorderType < 0)
        _columnBorderType = _rowBorderType;

    filter->rowBorderType = _rowBorderType;
    filter->columnBorderType = _columnBorderType;

    assert(columnBorderType != BORDER_WRAP);

    if(!filter->filter2D)
    {
        assert(filter->rowFilter && filter->columnFilter);
        filter->ksize = init_Point(rowFilter->ksize, columnFilter->ksize);
        filter->anchor = init_Point(rowFilter->anchor, columnFilter->anchor);
    }
    else
    {
        assert(filter->bufType == filter->srcType);
        filter->ksize = filter2D->ksize;
        filter->anchor = filter2D->anchor;
    }

    assert(0 <= anchor.x && anchor.x < ksize.x &&
           0 <= anchor.y && anchor.y < ksize.y);

    filter->borderElemSize = srcElemSize/(CV_MAT_DEPTH(filter->srcType) >= CV_32S ? sizeof(int) : 1);
    int borderLength = max(filter->ksize.x-1, 1);
    filter->borderTab = malloc(sizeof(vector));
    vector_init(filter->borderTab);
    vector_resize(filter->borderTab, borderLength* filter->borderElemSize);

    filter->maxWidth = filter->bufStep = 0;

    if(filter->rowBorderType == BORDER_CONSTANT || filter->columnBorderType == BORDER_CONSTANT)
    {
        filter->constBorderValue = malloc(sizeof(vector));
        vector_init(filter->constBorderValue);
        vector_resize(filter->constBorderValue, srcElemSize*borderLength);
        int srcType1 = CV_MAKETYPE(CV_MAT_DEPTH(filter->srcType), min(CV_MAT_CN(filter->srcType), 4));
        scalartoRawData(_borderValue, (unsigned char*)vector_get(filter->constBorderValue, 0), srcType1,
                        borderLength*CV_MAT_CN(filter->srcType));
    }
    filter->wholeSize = init_Point(-1, -1);
}

BaseFilter* init_BaseFilter(const Mat _kernel, Point _anchor,
        double _delta, const FilterVec_32f _vecOp)
{
    BaseFilter* f = malloc(sizeof(BaseFilter));
    f->anchor = _anchor;
    f->ksize = init_Point(_kernel.rows, _kernel.cols);
    f->delta = (float)_delta;
    f->vecOp = _vecOp;
    preprocess2DKernel(_kernel, f->coords, f->coeffs);
    vector_resize(f->ptrs, vector_size(coords));
    return f;
}

void preprocess2DKernel(const Mat kernel, vector* coords/*Point*/, vector* coeffs/*unsigned char*/)
{
    int i, j, k, nz = countNonZero(kernel), ktype = 5;
    if(nz == 0)
        nz = 1;

    vector_resize(coords, nz);
    vector_resize(coeffs, nz*(size_t)CV_ELEM_SIZE(ktype));
    unsigned char* _coeffs = (unsigned char*)vector_get(coeffs, 0);

    for(i = k = 0; i < kernel.rows; i++)
    {
        const unsigned char* krow = ptr(kernel, i);
        for(j = 0; j < kernel.cols; j++)
        {
            float val = ((const float*)krow)[j];
            if(val == 0)
                continue;
            vector_set(coords, k, init_Point(j, i));
            ((float*)_coeffs)[k++] = val;
        }
    }
}

FilterVec_32f filterVec_32f(Mat _kernel, double _delta)
{
    FilterVec_32f f;
    f.delta = (float)_delta;
    vector* coords = malloc(sizeof(vector)); //Point
    vector_init(coords);
    f.coeffs = malloc(sizeof(vector));
    vector_init(f.coeffs);
    preprocess2DKernel(_kernel, coords, f.coeffs);
    f._nz = (int)vector_size(coords);
    return f; 
}

BaseFilter* getLinearFilter(int srcType, int dstType, Mat kernel, Point anchor,
                                double delta, int bits)
{
    int sdepth = CV_MAT_DEPTH(srcType), ddepth = CV_MAT_DEPTH(dstType);
    int cn = CV_MAT_CN(srcType), kdepth = 5;

    anchor = normalizeAnchor(&anchor, init_Point(kernel.rows, kernel.cols));
    kdepth = sdepth = 5;

    BaseFilter* f = init_BaseFilter(kernel, anchor, delta, filterVec_32f(kernel, delta));
    return f;
}

//! returns the non-separable linear filter engine
FilterEngine* createLinearFilter(int _srcType, int _dstType, Mat kernel, Point _anchor, double _detla, int _rowBorderType, int _columnBorderType, Scalar _borderValue)
{
    _srcType = CV_MAT_TYPE(_srcType);
    _dstType = CV_MAT_TYPE(_dstType);
    int cn = CV_MAT_CN(_srcType);
    assert(cn == CV_MAT_CN(_dstType));

    int bits = 0;

    BaseFilter* filter2D = getLinearFilter(_srcType, _dstType, kernel, _anchor, _delta, bits);

    BaseDimFilter *row_filter = malloc(sizeof(BaseDimFilter)), *col_filter = malloc(sizeof(BaseDimFilter));
    row_filter->ksize = row_filter->anchor = -1;
    col_filter->ksize = col_filter->anchor = -1;

    FilterEngine* filter;
    init_FilterEngine(filter, _filter2D, row_filter, col_filter, _srcType, _dstType, _srcType,
        _rowBorderType, _columnBorderType, _borderValue);

    return filter; // Check ScalartoRawData code.
}

int Start(FilterEngine* f, Point _wholeSize, Point sz, Point ofs)
{
    int i, j;

    f->wholeSize = _wholeSize;
    f->roi = init_Rect(ofs.x, ofs.y, sz.y, sz.x);

    int esz = (int)getElemSize(srcType);
    int bufElemSize = (int)getElemSize(bufType);
    const unsigned char* constVal = !vector_empty(f->constBorderValue) ? (unsigned char*)vector_get(f->constBorderValue, 0) : 0;

    int _maxBufRows = max(f->ksize.y + 3,
                               max(f->anchor.y, f->ksize.y-anchor.y-1)*2+1);

    if(f->maxWidth < f->roi.width || _maxBufRows != vector_size(f->rows))
    {
        vector_resize(f->rows, _maxBufRows);
        maxWidth = max(f->maxWidth, f->roi.width);
        int cn = CV_MAT_CN(f->srcType);
        vector_resize(f->srcRow, esz*(f->maxWidth + f->ksize.x - 1));
        if(f->columnBorderType == 0)
        {
            assert(constVal != NULL);
            vector_resize(f->constBorderRow, (getElemSize(bufType)*(maxWidth + ksize.width - 1 + 64)));
            unsigned char *dst = alignptr((unsigned char)vector_get(f->constBorderRow, 0), 64), *tdst;
            int n = vector_size(f->constBorderValue), N;
            N = (f->maxWidth + f->ksize.x - 1)*esz;
            tdst = isSeparable(*f) ? (unsigned char*)vector_get(f->srcRow, 0) : dst;

            for(i = 0; i < N; i += n)
            {
                n = min(n, N - i);
                for(j = 0; j < n; j++)
                    tdst[i+j] = constVal[j];
            }
        }

        int maxBufStep = bufElemSize*(int)alignSize(maxWidth +
            (!isSeparable(*f) ? f->ksize.x - 1 : 0),VEC_ALIGN);
        vector_resize(f->ringBuf, maxBufStep*vector_size(f->rows)+VEC_ALIGN);
    }

    // adjust bufstep so that the used part of the ring buffer stays compact in memory
    bufStep = bufElemSize*(int)alignSize(f->roi.width + (!isSeparable(*f) ? f->ksize.x - 1 : 0),16);

    f->dx1 = max(f->anchor.x - f->roi.x, 0);
    f->dx2 = max(f->ksize.x - f->anchor.x - 1 + f->roi.x + f->roi.width - wholeSize.y, 0);

    // recompute border tables
    if(f->dx1 > 0 || f->dx2 > 0)
    {
        if(f->rowBorderType == 0)
        {
            int nr = isSeparable(*f) ? 1 : vector_size(f->rows);
            for(i = 0; i < nr; i++)
            {
                unsigned char* dst = isSeparable(*f) ? (unsigned char*)vector_get(f->srcRow, 0) : alignptr((unsigned char*)vector_get(f->ringBuf, 0), 64) + f->bufStep*i;
                memcpy(dst, constVal, f->dx1*esz);
                memcpy(dst + (f->roi.width + f->ksize.x - 1 - f->dx2)*esz, constVal, f->dx2*esz);
            }
        }
        else
        {
            int xofs1 = min(f->roi.x, f->anchor.x) - f->roi.x;

            int btab_esz = f->borderElemSize, wholeWidth = f->wholeSize.y;
            int* btab = (int*)vector_get(f->borderTab, 0);

            for(i = 0; i < f->dx1; i++)
            {
                int p0 = (borderInterpolate(i-f->dx1, wholeWidth, f->rowBorderType) + xofs1)*btab_esz;
                for(j = 0; j < btab_esz; j++)
                    btab[i*btab_esz + j] = p0 + j;
            }

            for(i = 0; i < f->dx2; i++)
            {
                int p0 = (borderInterpolate(wholeWidth + i, wholeWidth, f->rowBorderType) + xofs1)*btab_esz;
                for(j = 0; j < btab_esz; j++)
                    btab[(i + f->dx1)*btab_esz + j] = p0 + j;
            }
        }
    }

    f->rowCount = f->dstY = 0;
    f->startY = f->startY0 = max(f->roi.y - f->anchor.y, 0);
    f->endY = min(f->roi.y + f->roi.height + f->ksize.y - f->anchor.y - 1, f->wholeSize.x);

    if(f->columnFilter)
        free(f->columnFilter);
    if(f->filter2D)
        free(f->filter2D);

    return f->startY;
}

int start(FilterEngine* f, const Mat src, const Point wsz, const Point ofs)
{
    Start(f, wsz, init_Point(src.rows, src.cols), ofs);
    return f->startY - ofs.y;
}

int remainingInputRows(FilterEngine f)
{
    return f.endY - f.startY - f.rowCount;
}

int FilterVec_32f_op(FilterVec_32f* filter, const unsigned char** _src, unsigned char* _dst, int width)
{
    const float* kf = (const float*)vector_get(filter->coeffs, 0);
    const float** src = (const float**)_src;
    float* dst = (float*)_dst;
    int i = 0, k, nz = filter->_nz;
    __m128 d4 = _mm_set1_ps(filter->delta);

    for(; i <= width - 16; i += 16)
    {
        __m128 s0 = d4, s1 = d4, s2 = d4, s3 = d4;

        for( k = 0; k < nz; k++ )
        {
            __m128 f = _mm_load_ss(kf+k), t0, t1;
            f = _mm_shuffle_ps(f, f, 0);
            const float* S = src[k] + i;

            t0 = _mm_loadu_ps(S);
            t1 = _mm_loadu_ps(S + 4);
            s0 = _mm_add_ps(s0, _mm_mul_ps(t0, f));
            s1 = _mm_add_ps(s1, _mm_mul_ps(t1, f));

            t0 = _mm_loadu_ps(S + 8);
            t1 = _mm_loadu_ps(S + 12);
            s2 = _mm_add_ps(s2, _mm_mul_ps(t0, f));
            s3 = _mm_add_ps(s3, _mm_mul_ps(t1, f));
        }

        _mm_storeu_ps(dst + i, s0);
        _mm_storeu_ps(dst + i + 4, s1);
        _mm_storeu_ps(dst + i + 8, s2);
        _mm_storeu_ps(dst + i + 12, s3);
    }

    for( ; i <= width - 4; i += 4 )
    {
        __m128 s0 = d4;

        for( k = 0; k < nz; k++ )
        {
            __m128 f = _mm_load_ss(kf+k), t0;
            f = _mm_shuffle_ps(f, f, 0);
            t0 = _mm_loadu_ps(src[k] + i);
            s0 = _mm_add_ps(s0, _mm_mul_ps(t0, f));
        }
        _mm_storeu_ps(dst + i, s0);
    }

    return i;
}

void filter_op(BaseFilter* f, const unsigned char** src, unsigned char* dst, int dststep, int count, int width, int cn)
{
    float _delta = f->delta;
    const Point* pt = (Point*)vector_get(f->coords, 0);
    const float* kf = (const float*)vector_get(f->coeffs, 0);
    const float** kp = (const float**)(&((float*)vector_get(ptrs, 0)));
    int i, k, nz = vector_size(f->coords);

    width *= cn;
    for(; count > 0; count--, dst += dststep, src++)
    {
        float* D = (float*)dst;

        for(k = 0; k < nz; k++)
            kp[k] = (const float*)src[pt[k].y] + pt[k].x*cn;

        i = FilterVec_32f_op((const unsigned char**)kp, dst, width);
        for( ; i < width; i++ )
        {
            float s0 = _delta;
            for(k = 0; k < nz; k++)
                s0 += kf[k]*kp[k][i];
            D[i] = s0;
        }
    }
}

int proceed(FilterEngine* f, const unsigned char* src, int srcstep, int count,
                           unsigned char* dst, int dststep)
{
    const int *btab = (int*)vector_get(f->borderTab, 0);
    int esz = (int)getElemSize(f->srcType), btab_esz = f->borderElemSize;
    unsigned char** brows = vector_get(f->rows, 0);
    int bufRows = vector_size(f->rows);
    int cn = CV_MAT_CN(f->bufType);
    int width = f->roi.width, kwidth = f->ksize.x;
    int kheight = ksize.y, ay = f->anchor.y;
    int _dx1 = f->dx1, _dx2 = f->dx2;
    int width1 = f->roi.width + kwidth - 1;
    int xofs1 = min(f->roi.x, f->anchor.x);
    bool isSep = isSeparable(*f);
    bool makeBorder = (_dx1 > 0 || _dx2 > 0) && f->rowBorderType != BORDER_CONSTANT;
    int dy = 0, i = 0;

    src -= xofs1*esz;
    count = min(count, remainingInputRows(*f));

    assert(src && dst && count > 0);

    for(;; dst += dststep*i, dy += i)
    {
        int dcount = bufRows - ay - f->startY - f->rowCount + f->roi.y;
        dcount = dcount > 0 ? dcount : bufRows - kheight + 1;
        dcount = min(dcount, count);
        count -= dcount;

        for(; dcount-- > 0; src += srcstep)
        {
            int bi = (f->startY - f->startY0 + f->rowCount) % bufRows;
            unsigned char* brow = alignptr((unsigned char*)vector_get(f->ringBuf, 0), 64) + bi*f->bufStep;
            unsigned char* row = isSep ? (unsigned char*)vector_get(f->srcRow, 0) : brow;

            if(++f->rowCount > bufRows)
            {
                --(f->rowCount);
                ++(f->startY);
            }

            memcpy(row + _dx1*esz, src, (width1 - _dx2 - _dx1)*esz);

            if(makeBorder)
            {
                if(btab_esz*(int)sizeof(int) == esz)
                {
                    const int* isrc = (const int*)src;
                    int* irow = (int*)row;

                    for(i = 0; i < _dx1*btab_esz; i++)
                        irow[i] = isrc[btab[i]];
                    for(i = 0; i < _dx2*btab_esz; i++)
                        irow[i + (width1 - _dx2)*btab_esz] = isrc[btab[i+_dx1*btab_esz]];
                }
                else
                {
                    for(i = 0; i < _dx1*esz; i++)
                        row[i] = src[btab[i]];
                    for(i = 0; i < _dx2*esz; i++)
                        row[i + (width1 - _dx2)*esz] = src[btab[i+_dx1*esz]];
                }
            }
        }

        int max_i = min(bufRows, f->roi.height - (f->dstY + dy) + (kheight - 1));
        for(i = 0; i < max_i; i++)
        {
            int srcY = borderInterpolate(f->dstY + dy + i + f->roi.y - ay,
                            wholeSize.height, columnBorderType);
            if(srcY < 0) // can happen only with constant border type
                brows[i] = alignptr((unsigned char*)vector_get(f->constBorderRow, 0), 64);
            else
            {
                assert(srcY >= f->startY);
                if(srcY >= f->startY + f->rowCount)
                    break;
                int bi = (srcY - f->startY0) % bufRows;
                brows[i] = alignptr((unsigned char*)vector_get(f->ringBuf, 0), 64) + bi*bufStep;
            }
        }

        if(i < kheight)
            break;
        i -= kheight - 1;
        filter_op(f->filter2D, (const unsigned char**)brows, dst, dststep, i, roi.width, cn);
    }

    f->dstY += dy;
    assert(f->dstY <= f->roi.height);
    return dy;
}

void apply(FilterEngine* f, Mat src, Mat* dst, Point wsz, Point ofs)
{
    int y = start(f, src, wsz, ofs);
    proceed(f, ptr(src, 0) + y*src.step,
            (int)src.step,
            f->endY - f->startY,
            ptr(*dst, 0),
            (int)dst->step);
}