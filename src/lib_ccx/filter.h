ifndef FILTER_H
#define FILTER_H

#include "Mat.h"
#include "Math.h"

typedef struct BaseFilter
{
    Point ksize;
    Point anchor;
    vector* coords; // Point
    vector* coeffs; //unsigned char
    vector* ptrs; //unsigned char*
    float delta;
    FilterVec_32f vecOp;
} BaseFilter ;

typedef struct BaseDimFilter
{
    int ksize;
    int anchor;
} BaseDimFilter ;

typedef struct FilterEngine
{
    int srcType;
    int dstType;
    int bufType;
    Point ksize;
    Point anchor;
    int maxWidth;
    Point wholeSize;
    Rect roi;
    int dx1;
    int dx2;
    int rowBorderType;
    int columnBorderType;
    vector* borderTab; //int
    int borderElemSize;
    vector* ringBuf; //unsigned char
    vector* srcRow; //unsigned char
    vector* constBorderValue; //unsigned char
    vector* constBorderRow; //unsigned char
    int bufStep;
    int startY;
    int startY0;
    int endY;
    int rowCount;
    int dstY;
    vector* rows;// unsigned char*

    BaseFilter* filter2D;
    BaseDimFilter* rowFilter;
    BaseDimFilter* columnFilter;
} FilterEngine ;

typedef struct FilterVec_32f
{
    int _nz;
    vector* coeffs; // unsigned char
    float delta;
} FilterVec_32f ;

bool isSeparable(FilterEngine f)
{
    return !f.filter2D;
}

static inline size_t alignSize(size_t sz, int n)
{
    return (sz + n-1) & -n;
}

void init_FilterEngine(FilterEngine* filter, BaseFilter* _filter2D, BaseDimFilter* _rowFilter, BaseDimFilter* _columnFilter,
                       int _srcType, int _dstType, int _bufType,
                       int _rowBorderType/* BORDER_REPLICATE */, int _columnBorderType/*-1*/, const Scalar _borderValue /* init_Scalar(0, 0, 0, 0) */);

BaseFilter* init_BaseFilter(const Mat _kernel, Point _anchor,
        double _delta, const FilterVec_32f _vecOp);

FilterVec_32f filterVec_32f(Mat _kernel, double _delta);
BaseFilter* getLinearFilter(int srcType, int dstType, Mat kernel, Point anchor, double delta, int bits);
FilterEngine* createLinearFilter(int _srcType, int _dstType, Mat kernel, Point _anchor, double _detla, int _rowBorderType, int _columnBorderType, Scalar _borderValue);
int Start(FilterEngine* f, Point _wholeSize, Point sz, Point ofs);
int start(FilterEngine* f, const Mat src, const Point wsz, const Point ofs);
int remainingInputRows(FilterEngine f);
int FilterVec_32f_op(FilterVec_32f* filter, const unsigned char** _src, unsigned char* _dst, int width);
void filter_op(BaseFilter* f, const unsigned char** src, unsigned char* dst, int dststep, int count, int width, int cn);
int proceed(FilterEngine* f, const unsigned char* src, int srcstep, int count, unsigned char* dst, int dststep);
void apply(FilterEngine* f, Mat src, Mat* dst, Point wsz, Point ofs);
static void getElemSize(const char* fmt, size_t* elemSize, size_t* cn);

#endif