#include "Mat.h"
#include "math.h"
#include "filter.h"

#define CV_SPLIT_MERGE_MAX_BLOCK_SIZE(cn) ((INT_MAX/4)/cn)
#define CV_TOGGLE_FLT(x) ((x)^((int)(x) < 0 ? 0x7fffffff : 0))

Mat emptyMat()
{
    Mat m;
    m.rows = m.cols = 0;
    m.data = 0;
    m.datastart = 0;
    m.dataend = 0;
    m.datalimit = 0;
    m.flags = 0x42FF0000;
    m.step = 0;
    return m;
}

void seek(MatConstIterator* it, ptrdiff_t ofs, bool relative)
{
    if(isContinuous(&(it->m)))
    {
        if(it->ptr < it->sliceStart)
            it->ptr = it->sliceStart;
        else if(it->ptr > it->sliceEnd)
            it->ptr = it->sliceEnd;
        return;
    }

    ptrdiff_t ofs0, y;
    if(relative)
    {
        ofs0 = it->ptr - ptr(*(it->m), 0);
        y = ofs0/it->m->step[0];
        ofs += y*it->m->cols + (ofs0 - y*it->m->step[0])/elemSize;
    }

    y = ofs/m->cols;
    int y1 = min(max((int)y, 0), it->m->rows-1);
    it->sliceStart = ptr(*(it->m), y1);
    it->sliceEnd = it->sliceStart + it->m->cols*it->elemSize;
    it->ptr = y < 0 ? it->sliceStart : y >= it->m->rows ? it->sliceEnd :
    it->sliceStart + (ofs - y*it->m->cols)*it->elemSize;
    return;
}

void leftshift_op(Mat* m, int cols, float sample[])
{
    MatConstIterator it;
    it.m = malloc(sizeof(Mat));
    it.ptr = malloc(sizeof(unsigned char));
    it.sliceStart = malloc(sizeof(unsigned char));
    it.sliceEnd = malloc(sizeof(unsigned char));
    it.elemSize = elemSize(*m);
    it.m = m;
    it.ptr = 0;
    it.sliceStart = 0;
    it.sliceEnd = 0;


    if(isContinuous(*m))
    {
        it.sliceStart = ptr(m, 0);
        it.sliceEnd = it.sliceStart + m->rows * m->cols * it.elemSize;
    }
    seek(&it, 0, false);

    for(int i = 0; i < cols; i++)
    {
        *(float*)(it.ptr) = float(sample[i]);
        if(it.m && (it.ptr += it.elemSize) >= it.sliceEnd)
        {
            it.ptr -= it.elemSize;
            seek(&it, 1, true);
        }
    }
}

int updateContinuityFlag(Mat* m)
{
    int i, j;
    int sz[] = {m->rows, m->cols};
    
    for(i = 0; i < 2; i++)
    {
        if(sz[i] > 1)
            break;
    }
    
    uint64_t t = (uint64_t)sz[i]*CV_MAT_CN(m->flags);
    for(j = 1; j > i; j--)
    {
        t *= sz[j];
        if(m->step[j]*sz[j] < m->step[j-1] )
            break;
    }

    if(j <= i && t == (uint64_t)(int)t)
        return m->flags | 16384;

    return m->flags & ~16384;
}

size_t elemSize(Mat m)
{
    return m.step[1];
}

size_t elemSize1(Mat m)
{
    return CV_ELEM_SIZE1(flags);
}

bool isContinuous(Mat m)
{
    return (m.flags & 16384) != 0;
}

bool isSubmatrix(Mat m)
{
    return (m.flags & (1<<15)) != 0;
}

void release(Mat* m)
{
    if(!m)
        return;
    m->datastart = m->dataend = m->datalimit = m->data = 0;
    m->rows = m->cols = 0;
    free(m->step);
}

int type(Mat m)
{
    return CV_MAT_TYPE(m.flags);
}

inline bool empty(Mat m)
{
    return (m.data == 0) || (m.rows*m.cols == 0);
}

int depth(Mat m)
{
    return CV_MAT_DEPTH(m.flags);
}

int channels(Mat m)
{
    return CV_MAT_CN(m.flags);
}

int total(Mat m)
{
    return (size_t)m.rows * m.cols;
}

unsigned char* ptr(Mat m, int i)
{
    return m.data + m.step[0] * i;
}

int checkVector(Mat m, int _elemChannels, int _depth)
{
    return m.data && (depth(m) == _depth || _depth <= 0) && isContinuous(m) &&
    (((m.rows == 1 || m.cols == 1) && channels(m) == _elemChannels) ||
                        (m.cols == _elemChannels && channels(m) == 1))?(total(m)*channels(m)/_elemChannels) : -1;
}

Point getContinuousSize_(int flags, int cols, int rows, int widthScale)
{
    int64_t sz = (int64_t)cols * rows * widthScale;
    return (flags & 16384) != 0 &&
        (int)sz == sz ? init_Point((int)sz, 1) : init_Point(cols * widthScale, rows);
}

Point getContinuousSize(Mat m1, Mat m2, int widthScale)
{
    return getContinuousSize_(m1.flags & m2.flags,
                              m1.cols, m1.rows, widthScale);
}

void copyTo(Mat src, Mat* dst)
{    
    if(empty(src))
    {
        release(dst);
        return;
    }

    create(dst, src.rows, src.cols, type(src));
    if(src.data == dst->data)
            return;

    if(src.rows > 0 && src.cols > 0)
    {
        const unsigned char* sptr = src.data;
        unsigned char* dptr = dst->data;

        Point sz = getContinuousSize(src, dst, 1);
        size_t len = sz.x*elemSize(src);

        for(; sz.y--; sptr += src.step, dptr += dst->step)
            memcpy(dptr, sptr, len);
    }
    return;
}

void cvtScale_8u(const unsigned char* src, size_t sstep, const unsigned char* dst, size_t dstep, Point size, float scale, float shift)
{
    sstep /= sizeof(src[0]);
    dstep /= sizeof(dst[0]);

    for(;size.y--; src += sstep, dst += dstep)
    {
        int x = 0;

        for(; x < size.x; x++ )
            dst[x] = (unsigned char)(src[x]*scale + shift);
    }
}

void cvt32f8u(const float* src, size_t sstep, unsigned char* dst, size_t dstep, Point size)
{
    sstep /= sizeof(src[0]);
    dstep /= sizeof(dst[0]);

    for(; size.y--; src += sstep, dst += dstep)
    {
        int x = 0;
        for(; x < size.x; x++)
            dst[x] = (unsigned char)src[x];
    }
}

void cvt8u32f(const unsigned char* src, size_t sstep, float* dst, size_t dstep, Point size)
{
    sstep /= sizeof(src[0]);
    dstep /= sizeof(dst[0]);

    for(; size.y--; src += sstep, dst += dstep)
    {
        int x = 0;
        for(; x < size.x; x++)
            dst[x] = (float)src[x];
    }
}

void convertTo(Mat src, Mat* dst, int _type, double alpha/*1*/, double beta/*0*/, int code)
{
    if(empty(src))
    {
        release(dst);
        return;
    }

    bool noScale = fabs(alpha-1) == 0 && fabs(beta) == 0;
    if(_type < 0)
        _type = type(src);
    else
         _type = CV_MAKETYPE(CV_MAT_DEPTH(_type), channels(src));

    int sdepth = depth(src), ddepth = CV_MAT_DEPTH(_type);
    if(sdepth == ddepth && noScale)
    {
        copyTo(src, dst);
        return;
    }

    create(dst, src.rows, src.cols, _type);

    double scale[] = {alpha, beta};
    int cn = channels(src);

    Point sz = getContinuousSize(src, *dst, cn);

    if(code == 0)
    { 
        cvt8u32f(src.data, src.step, dst->data, dst->step, sz);
        return;
    }

    if(code == 1)
    {
        cvt32f8u(src.data, src.step, dst->data, dst->step, sz);
        return;
    }

    if(code == 2)
    {
        cvtScale_8u(src.data, src.step, dst->data, dst->step, sz, (float)scale[0], (float)scale[1]);
        return;
    }

    //else
    //   cpy_8u(src.data, src.step, dst->data, dst->step, sz);
}

inline void create(Mat* m, int _rows, int _cols, int _type)
{
	_type &= TYPE_MASK;
    if(m->rows == _rows && m->cols == _cols && type(*m) == _type && m->data)
        return;

    int sz[] = {_rows, _cols};
    _type = CV_MAT_TYPE(_type);
    
    release(m);
    m->rows = _rows;
    m->cols = _cols;
    m->flags = (_type & CV_MAT_TYPE_MASK) | MAGIC_VAL;
    m->data = 0;

    size_t esz = CV_ELEM_SIZE(m->flags), total = esz;
    for(int i = 1; i >= 0; i--)
    {
        m->step[i] = total;
        signed __int64 total1 = (signed __int64)total*sz[i];
        if((unsigned __int64)total1 != (size_t)total1)
            fatal("The total matrix size does not fit to \"size_t\" type");
        total = (size_t)total1;
    }
    if(m->rows * m->cols > 0)
    {
        size_t total_ = CV_ELEM_SIZE(_type);
        for(int i = 1; i >= 0; i++)
        {
            if(m->step)
                m->step[i] = total_;

            total_ *= sizes[i];
        }
        unsigned char* udata = malloc(size + sizeof(void*) + CV_MALLOC_ALIGN);
        if(!udata)
            fatal("Out of Memory Error");
        unsigned char** adata = alignPtr((unsigned char**)udata + 1, CV_MALLOC_ALIGN);
        adata[-1] = udata;
        m->datastart = m->data = (unsigned char*)adata;
        assert(m->step[1] == (size_t)CV_ELEM_SIZE(flags));
    }
    m->flags = updateContinuityFlag(m);
    if(m->data)
    {
        m->datalimit = m->datastart + m->rows*m->step[0];
        if(m->rows > 0)
        {
            m->dataend = ptr(*m, 0) + m->cols*m->step[1];
            m->dataend += (m->rows - 1)*m->step[0];
        }
        else
            m->dataend = m->datalimit;
    }
    else
        m->dataend = m->datalimit = 0;
}

Mat createusingRect(Mat _m, Rect roi)
{
    Mat m;
    m.flags = _m.flags;
    m.rows = roi.height;
    m.cols = roi.width;
    m.datastart = _m.datastart;
    m.datalimit = _m.datalimit;
    m.dataend = _m.dataend;
    m.data = _m.data + roi.y*_m.step[0];
    size_t esz = CV_ELEM_SIZE(m.flags);
    m.data += roi.x*esz;
    
    if(roi.width < _m.cols || roi.height < _m.rows)
        m.flags |= (1<<15);
    
    m.step[0] = _m.step[0]; m.step[1] = esz;
    m.flags = updateContinuityFlag(&m);
    
    if(m.rows <= 0 || m.cols <= 0)
    {
        release(&m);
        m.rows = m.cols = 0;
    }

    return m;
}

// Sets all or some of the array elements to the specified value.
void createusingScalar(Mat* m, Scalar s)
{
    const Mat* arrays[] = {m};
    unsigned char* dptr;
    MatIterator it = matIterator(arrays, &dptr, 1);
    size_t elsize = it.size*elemSize(*m);
    const int64_t* is = (const int64_t*)&s.val[0];

    if(is[0] == 0 && is[1] == 0 && is[2] == 0 && is[3] == 0)
    {
        for(size_t i = 0; i < it.nplanes; i++, getNextIterator(&it))
            memset(dptr, 0, elsize);
    }
    /*
    else
    {
        if(it.nplanes > 0)
        {
            double scalar[12];
            scalartoRawData(s, scalar, type(*m), 12);
            size_t blockSize = 12*elemSize1(*m);

            for(size_t j = 0; j < elsize; j += blockSize)
            {
                size_t sz = min(blockSize, elsize - j);
                memcpy(dptr+j, scalar, sz);
            }
        }

        for(size_t i = 1; i < it.nplanes; i++, getNextIterator(&it))
            memcpy(dptr, m->data, elsize);
    }
    */
}

inline Mat createusingPoint(vector* v)
{
    Mat m;
    m.flags = 0x42FF0000 | 12 | (1<<14);
    m.rows = vector_size(v);
    m.cols = 1;
    m.data = m.datastart = m.dataend = m.datalimit = 0;
    if(vector_empty(v))
        return m;
    
    Point* arr = malloc(sizeof(Point)*vector_size(v));
    for(int i = 0;i < vector_size(v);i++)
        arr[i] = *(Point*)vector_get(v, i);

    m.step[0] = m.step[1] = sizeof(Point);
    m.datastart = m.data = (unsigned char*)&arr[0];
    m.datalimit = m.dataend = m.datastart + m.rows * m.step[0];

    return m;
}

Mat createMat(int _rows, int _cols, int _type, void* _data, size_t _step)
{
    Mat m;
    create(&m, _rows, _cols, _type);
    m.data = (unsigned char*)_data;
    m.datastart = (unsigned char*)_data;
    m.dataend = 0;
    m.datalimit = 0;

    size_t esz = CV_ELEM_SIZE(_type), esz1 = CV_ELEM_SIZE1(_type);
    size_t minstep = _cols*esz;


    if(_step == 0)
        _step = minstep;

    else
    {
        if (_step % esz1 != 0)
        {
            fatal("Step must be a multiple of esz1");
        }
    }

    m.step[0] = _step;
    m.step[1] = esz;
    m.datalimit = m.datastart + _step*m.rows;
    m.dataend = m.datalimit - _step + minstep;
    m.flags = updateContinuityFlag(&m);
    return m;
}

Mat createusingRange(Mat m, int row0, row1)
{
    Mat src = m;
    if(row0 != 0 && row1 != src.rows)
    {
        assert(0 <= row0 && row0 <= row1
                       && row1 <= m.rows);
        m.rows = row1-row0;
        m.data += m.step*row0;
        m.flags |= (1<<15);
    }

    m.flags = updateContinuityFlag(&m);

    if(m.rows <= 0 || m->cols <= 0)
    {
        release(&m);
        m.rows = m.cols = 0;
    }

    return m;
}

Mat getMatfromVector(vector* v)
{
    return !vector_empty(v) ? createMat(1, vector_size(v), 12, (void*)vector_get(v, 0), 0) : emptyMat();
}

Mat getMatfromScalar(Scalar* s)
{
    return createMat(4, 1, -1040056314, s, 0);
}

Mat row_op(Mat m, Point rowRange)
{
    Mat sample;
    sample.flags = m.flags;
    sample.rows = m.rows;
    sample.cols = m.cols;
    sample.step[0] = m.step[0];
    sample.step[1] = m.step[1];
    sample.data = m.data;
    sample.datastart = m.datastart;
    sample.dataend = m.dataend;
    sample.datalimit = m.datalimit;

    if(rowRange.x != 0 && rowRange.y != sample.rows)
    {
        sample.rows = rowRange.y-rowRange.x;
        sample.data += sample.step*rowRange.x;
        sample.flags |= (1<<15);
    }
    m.flags = updateContinuityFlag(&sample);
    return sample;
}

static int sqsum8u(const unsigned char* src, const unsigned char* mask, int* sum, int* sqsum, int len, int cn)
{
    const unsigned char* src = src0;

    if(!mask)
    {
        int i = 0, k = cn % 4;
        src += i * cn;

        if(k == 1)
        {
            int s0 = sum[0];
            int sq0 = sqsum[0];
            for(; i < len; i++, src += cn)
            {
                int v = src[0];
                s0 += v; sq0 += (int)v*v;
            }
            sum[0] = s0;
            sqsum[0] = sq0;
        }
        else if( k == 2 )
        {
            int s0 = sum[0], s1 = sum[1];
            int sq0 = sqsum[0], sq1 = sqsum[1];
            for(; i < len; i++, src += cn)
            {
                unsigned char v0 = src[0], v1 = src[1];
                s0 += v0; sq0 += (int)v0*v0;
                s1 += v1; sq1 += (int)v1*v1;
            }
            sum[0] = s0; sum[1] = s1;
            sqsum[0] = sq0; sqsum[1] = sq1;
        }
        else if(k == 3)
        {
            int s0 = sum[0], s1 = sum[1], s2 = sum[2];
            int sq0 = sqsum[0], sq1 = sqsum[1], sq2 = sqsum[2];
            for(; i < len; i++, src += cn)
            {
                unsigned char v0 = src[0], v1 = src[1], v2 = src[2];
                s0 += v0; sq0 += (int)v0*v0;
                s1 += v1; sq1 += (int)v1*v1;
                s2 += v2; sq2 += (int)v2*v2;
            }
            sum[0] = s0; sum[1] = s1; sum[2] = s2;
            sqsum[0] = sq0; sqsum[1] = sq1; sqsum[2] = sq2;
        }

        for(; k < cn; k += 4)
        {
            src = src0 + k;
            int s0 = sum[k], s1 = sum[k+1], s2 = sum[k+2], s3 = sum[k+3];
            int sq0 = sqsum[k], sq1 = sqsum[k+1], sq2 = sqsum[k+2], sq3 = sqsum[k+3];
            for(; i < len; i++, src += cn)
            {
                unsigned char v0, v1;
                v0 = src[0], v1 = src[1];
                s0 += v0; sq0 += (int)v0*v0;
                s1 += v1; sq1 += (int)v1*v1;
                v0 = src[2], v1 = src[3];
                s2 += v0; sq2 += (int)v0*v0;
                s3 += v1; sq3 += (int)v1*v1;
            }
            sum[k] = s0; sum[k+1] = s1;
            sum[k+2] = s2; sum[k+3] = s3;
            sqsum[k] = sq0; sqsum[k+1] = sq1;
            sqsum[k+2] = sq2; sqsum[k+3] = sq3;
        }
        return len;
    }

    if(cn == 1)
    {
        int s0 = sum[0];
        int sq0 = sqsum[0];
        for( i = 0; i < len; i++ )
            if( mask[i] )
            {
                unsigned char v = src[i];
                s0 += v; sq0 += (int)v*v;
                nzm++;
            }
        sum[0] = s0;
        sqsum[0] = sq0;
    }
    else if(cn == 3)
    {
        int s0 = sum[0], s1 = sum[1], s2 = sum[2];
        int sq0 = sqsum[0], sq1 = sqsum[1], sq2 = sqsum[2];
        for(i = 0; i < len; i++, src += 3)
            if(mask[i])
            {
                unsigned char v0 = src[0], v1 = src[1], v2 = src[2];
                s0 += v0; sq0 += (int)v0*v0;
                s1 += v1; sq1 += (int)v1*v1;
                s2 += v2; sq2 += (int)v2*v2;
                nzm++;
            }
        sum[0] = s0; sum[1] = s1; sum[2] = s2;
        sqsum[0] = sq0; sqsum[1] = sq1; sqsum[2] = sq2;
    }
    else
    {
        for(i = 0; i < len; i++, src += cn)
            if(mask[i])
            {
                for(int k = 0; k < cn; k++)
                {
                    unsigned char v = src[k];
                    int s = sum[k] + v;
                    int sq = sqsum[k] + (int)v*v;
                    sum[k] = s; sqsum[k] = sq;
                }
                nzm++;
            }
    }
    return nzm;
}

void meanStdDev(Mat* src, Scalar* mean, Scalar* sdv, Mat* mask)
{
    int k, cn = channels(*src), depth = depth(*src);

    const Mat* arrays[] = {src, mask, 0};
    unsigned char* ptrs[2];
    MatIterator it = matIterator(arrays, ptrs, 2);
    int total = it.size, blockSize = total, intSumBlockSize = 0;
    int j, count = 0, nz0 = 0;
    AutoBuffer _buf = init_AutoBufferdouble(cn*4);
    double* s = (double*)_buf.ptr;, *sq = s + cn;
    int *sbuf = (int*)s, *sqbuf = (int*)sq;
    bool blockSum = depth <= 3, blockSqSum = depth <= 1;
    size_t esz = 0;

    for(k = 0; k < cn; k++)
        s[k] = sq[k] = 0;

    if(blockSum)
    {
        intSumBlockSize = 1 << 15;
        blockSize = min(blockSize, intSumBlockSize);
        sbuf = (int*)(sq + cn);
        if(blockSqSum)
            sqbuf = sbuf + cn;
        for(k = 0; k < cn; k++)
            sbuf[k] = sqbuf[k] = 0;
        esz = elemSize(*src);
    }

    for(size_t i = 0; i < it.nplanes; i++, getNextIterator(&it))
    {
        for(j = 0; j < total; j += blockSize)
        {
            int bsz = min(total - j, blockSize);
            int nz = sqsum8u(ptrs[0], ptrs[1], (unsigned char*)sbuf, (unsigned char*)sqbuf, bsz, cn);
            count += nz;
            nz0 += nz;
            if(blockSum && (count + blockSize >= intSumBlockSize || (i+1 >= it.nplanes && j+bsz >= total)))
            {
                for( k = 0; k < cn; k++ )
                {
                    s[k] += sbuf[k];
                    sbuf[k] = 0;
                }
                if( blockSqSum )
                {
                    for( k = 0; k < cn; k++ )
                    {
                        sq[k] += sqbuf[k];
                        sqbuf[k] = 0;
                    }
                }
                count = 0;
            }
            ptrs[0] += bsz*esz;
            if( ptrs[1] )
                ptrs[1] += bsz;
        }
    }

    double scale = nz0 ? 1./nz0 : 0.;
    for(k = 0; k < cn; k++)
    {
        s[k] *= scale;
        sq[k] = sqrt(max(sq[k]*scale - s[k]*s[k], 0.));
    }

    for(j = 0; j < 2; j++)
    {
        const double* sptr = j == 0 ? s : sq;
        Scalar* _dst = j == 0 ? mean : sdv;
        
        Mat dst = getMatfromScalar(_dst);

        int dcn = total(dst);
        double* dptr = (double*)ptr(dst, 0);
        for(k = 0; k < cn; k++)
            dptr[k] = sptr[k];
        for(; k < dcn; k++)
            dptr[k] = 0;
    }
}

void divide_op(Mat* m, double s)
{
    convertTo(*m, m, -1, 1.0/s, 0, 2);
}

void minus_op(Mat* m, double s, double alpha)
{
    convertTo(*m, m, -1, alpha, 0, 2);
    //add(*m, init_Scalar(-s, 0, 0, 0), m);
}

void zeros(Mat* m, int _rows, int _cols, int _type)
{
    *m = createMat(_rows, _cols, _type, (void*)(size_t)0xEEEEEEEE, 0);
    createusingScalar(m, init_Scalar(0, 0, 0, 0));     
}

void locateROI(Mat src, Point* wholeSize, Point* ofs)
{
    assert((src.step)[0] > 0);
    size_t esz = elemSize(src), minstep;
    ptrdiff_t delta1 = src.data - src.datastart, delta2 = src.dataend - src.datastart;

    if(delta1 == 0)
        ofs->x = ofs->y = 0;

    else
    {
        ofs->y = (int)(delta1/step[0]);
        ofs->x = (int)((delta1 - step[0]*ofs->y)/esz);
    }
    minstep = (ofs->x + cols)*esz;
    wholeSize->y = (int)((delta2 - minstep)/step[0] + 1);
    wholeSize->y = max(wholeSize->y, ofs->y + src.rows);
    wholeSize->x = (int)((delta2 - step*(wholeSize->y-1))/esz);
    wholeSize->x = max(wholeSize->x, ofs->x + src.cols);
}

void adjustROI(Mat* src, int dtop, int dbottom, int dleft, int dright)
{
    assert((src->step)[0] > 0);
    Point *wholesize = malloc(sizeof(Point)), *ofs;
    size_t esz = elemSize(*src);
    locateROI(*src, wholeSize, ofs);
    int row1 = min(max(ofs->y - dtop, 0), wholeSize->y), row2 = max(0, min(ofs->y + rows + dbottom, wholeSize->y));
    int col1 = min(max(ofs->x - dleft, 0), wholeSize->x), col2 = max(0, min(ofs->x + cols + dright, wholeSize->x));

    if(row1 > row2)
        swap(&row1, &row2);

    if(col1 > col2)
        swap(&col1, &col2);

    src->data += (row1 - ofs->y)*(m.step)[0] + (col1 - ofs->x)*esz;
    src->rows = row2 - row1; src->cols = col2 - col1;
    m.flags = updateContinuityFlag(src);
}

void createVectorOfVector(vector* v, int rows, int cols, int mtype, int i)
{
    size_t len = rows*cols > 0 ? rows + cols - 1 : 0;

    if(i < 0)
    {
        vector_resize(v, len);
        return;
    }

    vector_resize(vector_get(v, i), len);
}

static int countNonZero8u(const unsigned char* src, int len)
{
    int i=0, nz = 0;
    for( ; i < len; i++ )
        nz += src[i] != 0;
    return nz;
}

int countNonZero(Mat src)
{
    int type = type(src), cn = CV_MAT_CN(type);
    assert(cn == 1);

    const Mat* arrays[] = {&src, 0};
    unsigned char* ptrs[1];
    MatIterator it = matIterator(arrays, ptrs, 1);
    int total = (int)it.size, nz = 0;

    for( size_t i = 0; i < it.nplanes; i++, ++it )
        nz += countNonZero8u(ptrs[0], total);

    return nz;
}

void createVectorMat(vector* v, int rows, int cols, int mtype, int i)
{
    mtype = CV_MAT_TYPE(mtype);

    if(i < 0)
    {
        assert(rows == 1 || cols == 1 || rows*cols == 0);
        size_t len = rows*cols > 0 ? rows + cols - 1 : 0;

        vector_resize(v, len);
        return;
    }

    assert(i < vector_size(v));
    Mat* m = (Mat*)vector_get(v, i);

    create(m, rows, cols, mtype);
    return;
}

void getNextIterator(MatIterator* it)
{
    if(it->idx >= it->nplanes-1)
        return;
    ++(it->idx);

    if(it->iterdepth == 1)
    {
        for(int i = 0; i < it->narrays; i++)
        {
            if(!(it->ptrs[i]))
                continue;
            it->ptrs[i] = it->arrays[i]->data + it->arrays[i]->step[0]->idx;
        }
    }
    else
    {
        for(int i = 0; i < it->narrays; i++)
        {
            Mat* A = arrays[i];
            if(!A->data)
                continue;

            int sz[2] = {A->rows, A->cols};
            int _idx = (int)it->idx;
            unsigned char* data = A->data;
            for(int j = it->iterdepth-1; j >= 0 && _idx > 0; j--)
            {
                int szi = sz[j], t = _idx/szi;
                data += (_idx - t * szi)*A->step[j];
                _idx = t;
            }
            if(it->ptrs)
                it->ptrs[i] = data;
        }
    }
}

void split_(const unsigned char* src, unsigned char** dst, int len, int cn)
{
    int k = cn % 4 ? cn % 4 : 4;
    int i, j;
    if(k == 1)
    {
        unsigned char* dst0 = dst[0];

        if(cn == 1)
        {
            memcpy(dst0, src, len*sizeof(unsigned char));
        }
        else
        {
            for(i = 0, j = 0 ; i < len; i++, j += cn)
                dst0[i] = src[j];
        }
    }
    else if(k == 2)
    {
        unsigned char *dst0 = dst[0], *dst1 = dst[1];
        i = j = 0;

        for(; i < len; i++, j += cn)
        {
            dst0[i] = src[j];
            dst1[i] = src[j+1];
        }
    }
    else if(k == 3)
    {
        unsigned char *dst0 = dst[0], *dst1 = dst[1], *dst2 = dst[2];
        i = j = 0;

        for(; i < len; i++, j += cn)
        {
            dst0[i] = src[j];
            dst1[i] = src[j+1];
            dst2[i] = src[j+2];
        }
    }
    else
    {
        unsigned char *dst0 = dst[0], *dst1 = dst[1], *dst2 = dst[2], *dst3 = dst[3];
        i = j = 0;

        for(; i < len; i++, j += cn)
        {
            dst0[i] = src[j]; dst1[i] = src[j+1];
            dst2[i] = src[j+2]; dst3[i] = src[j+3];
        }
    }

    for(; k < cn; k += 4)
    {
        unsigned char *dst0 = dst[k], *dst1 = dst[k+1], *dst2 = dst[k+2], *dst3 = dst[k+3];
        for(i = 0, j = k; i < len; i++, j += cn)
        {
            dst0[i] = src[j]; dst1[i] = src[j+1];
            dst2[i] = src[j+2]; dst3[i] = src[j+3];
        }
    }
}

void split_channels(Mat src, Mat* mv)
{
    int k, depth = depth(src), cn = channels(src);
    if(cn == 1)
    {
        copyTo(src, &mv[0]);
        return;
    }

    for(k = 0; k < cn; k++)
        create(&mv[k], src.rows, src.cols, depth);

    size_t esz = elemSize(src), esz1 = elemSize1(src);
    size_t blocksize0 = (BLOCK_SIZE + esz-1)/esz;
    AutoBuffer _buf = init_AutoBufferuchar((cn+1)*(sizeof(Mat*) + sizeof(unsigned char*)) + 16);
    const Mat** arrays = (const Mat**)(unsigned char*)_buf.ptr;
    unsigned char** ptrs = (unsigned char**)alignPtr(arrays + cn + 1, 16);

    arrays[0] = &src;
    for(k = 0; k < cn; k++)
    {
        arrays[k+1] = &mv[k];
    }

    MatIterator it = matIterator(arrays, ptrs, cn+1);
    size_t total = it.size;
    size_t blocksize = min((size_t)CV_SPLIT_MERGE_MAX_BLOCK_SIZE(cn), cn <= 4 ? total : min(total, blocksize0));

    for(size_t i = 0; i < it.nplanes; i++, getNextIterator(&it))
    {
        for(size_t j = 0; j < total; j += blocksize)
        {
            size_t bsz = min(total - j, blocksize);
            split_(ptrs[0], &ptrs[1], (int)bsz, cn);

            if(j + blocksize < total)
            {
                ptrs[0] += bsz*esz;
                for(k = 0; k < cn; k++)
                    ptrs[k+1] += bsz*esz1;
            }
        }
    }
}

void split(Mat m, vector* dst)
{
    if(empty(m))
    {
        vector_free(dst);
        return;
    }

    int depth = depth(m);
    int cn = channels(m);

    createVectorMat(dst, cn, 1, depth, -1);
    for (int i = 0; i < cn; ++i)
        createVectorMat(dst, m.rows, m.cols, depth, i);

    Mat* mv = malloc(sizeof(Mat)*vector_size(dst));
    for(int i = 0; i < vector_size(dst); i++)
        mv[i] = *(Mat*)vector_get(dst, i);

    split_channels(m, &mv[0]);
    for(int i = 0; i < vector_size(dst); i++)
        vector_set(dst, i, &mv[i]);
}

void normalizeAnchor(Point* anchor, Point ksize)
{
    if(anchor->x < 0)
        anchor->x = ksize.y >> 1;
    
    if(anchor->y < 0)
        anchor->y = ksize.x >> 1;
}

static void ocvFilter2D(int stype, int dtype, int kernel_type,
                        unsigned char* src_data, size_t src_step,
                        unsigned char* dst_data, size_t dst_step,
                        int width, int height,
                        int full_width, int full_height,
                        int offset_x, int offset_y,
                        unsigned char* kernel_data, size_t kernel_step,
                        int kernel_width, int kernel_height,
                        int anchor_x, int anchor_y,
                        double delta, int borderType)
{
    int borderTypeValue = borderType & ~16;
    Mat kernel = createMat(kernel_height, kernel_width, kernel_type, kernel_data, kernel_step);
    FilterEngine* f = createLinearFilter(stype, dtype, kernel, init_Point(anchor_x, anchor_y), delta, borderTypeValue, -1, init_Scalar(0, 0, 0, 0));
    Mat src = createMat(height, width, stype, src_data, src_step);
    Mat dst = createMat(height, width, stype, dst_data, dst_step);
    apply(f, src, &dst, init_Point(full_height, full_width), init_Point(offset_x, offset_y));
}

void Filter2D(int stype, int dtype, int kernel_type,
              unsigned char* src_data, size_t src_step,
              unsigned char* dst_data, size_t dst_step,
              int width, int height,
              int full_width, int full_height,
              int offset_x, int offset_y,
              unsigned char* kernel_data, size_t kernel_step,
              int kernel_width, int kernel_height,
              int anchor_x, int anchor_y,
              double delta, int borderType)
{
    ocvFilter2D(stype, dtype, kernel_type,
                src_data, src_step,
                dst_data, dst_step,
                width, height,
                full_width, full_height,
                offset_x, offset_y,
                kernel_data, kernel_step,
                kernel_width, kernel_height,
                anchor_x, anchor_y,
                delta, borderType);
}


void filter2D(Mat src, Mat* dst, int ddepth, Mat kernel, Point anchor0, double delta, int borderType)
{
    if(ddepth < 0)
        ddepth = depth(src);

    create(dst, src.rows, src.cols, ((ddepth & 7) + (((channels(src))-1) << 3)));
    Point anchor = normalizeAnchor(&anchor0, init_Point(src.rows, src.cols));

    Point ofs;
    Point wsz = init_Point(src.rows, src.cols);
    if(borderType & 16 == 0)
        locateROI(src, &wsz, &ofs);

    Filter2D(5, 5, 5, src.data, src.step, dst->data, dst->step,
             dst->cols, dst->rows, wsz.y, wsz.x, ofs.x, ofs.y,
             kernel.data, kernel.step, kernel.cols, kernel.rows, 
             anchor.x, anchor.y
             delta, borderType);
}

void magnitude(Mat X, Mat Y, Mat* Mag)
{
    int type = type(X), depth = depth(X), cn = channels(X);
    create(Mag, X.rows, X.cols, type);

    const Mat* arrays[] = {&X, &Y, &Mag, 0};
    unsigned char* ptrs[3];
    MatIterator it = matIterator(arrays, ptrs, 3);
    int len = (int)it.size*cn;

    for(size_t i = 0; i < it.nplanes; i++, getNextIterator(&it))
    {
        if(depth == 5)
        {
            const float *x = (const float*)ptrs[0], *y = (const float*)ptrs[1];
            float *mag = (float*)ptrs[2];
            for(int j = 0; j < len; j++)
            {
                float x0 = x[j], y0 = y[j];
                mag[j] = sqrt(x0*x0 + y0*y0);
            }
        }
        else
        {
            const double *x = (const double*)ptrs[0], *y = (const double*)ptrs[1];
            double *mag = (double*)ptrs[2];
            for(int j = 0; j < len; j++)
            {
                double x0 = x[j], y0 = y[j];
                mag[j] = sqrt(x0*x0 + y0*y0);
            }
        }
    }
}

Mat mat_op(Mat mat)
{
    Mat m;
    m.flags = 1124007941;
    convertTo(mat, &m, 5, 1, 0, 0);
    return m;
}

void get_gradient_magnitude(Mat* _grey_image, Mat* _gradient_magnitude)
{
    Mat C = mat_op(*_grey_image);

    Mat kernel;
    create(&kernel, 1, 3, 5);
    float sample[3] = {-1, 0, 1};
    leftshift_op(&kernel, 3, sample);

    Mat grad_x;
    filter2D(C, grad_x, -1, kernel, init_Point(-1,-1), 0, 4);

    Mat kernel2;
    create(&kernel2, 3, 1, 5);
    leftshift_op(&kernel, 3, sample);
    Mat grad_y;
    filter2D(C, grad_y, -1, kernel2, init_Point(-1,-1), 0, 4);

    magnitude(grad_x, grad_y, _gradient_magnitude);
}

static int Sklansky_(Point** array, int start, int end, int* stack, int nsign, int sign2)
{
    int incr = end > start ? 1 : -1;
    // prepare first triangle
    int pprev = start, pcur = pprev + incr, pnext = pcur + incr;
    int stacksize = 3;

    if( start == end ||
       (array[start]->x == array[end]->x &&
        array[start]->y == array[end]->y) )
    {
        stack[0] = start;
        return 1;
    }

    stack[0] = pprev;
    stack[1] = pcur;
    stack[2] = pnext;

    end += incr; // make end = afterend

    while( pnext != end )
    {
        // check the angle p1,p2,p3
        int cury = array[pcur]->y;
        int nexty = array[pnext]->y;
        int by = nexty - cury;

        if(CV_SIGN(by) != nsign)
        {
            int ax = array[pcur]->x - array[pprev]->x;
            int bx = array[pnext]->x - array[pcur]->x;
            int ay = cury - array[pprev]->y;
            int convexity = ay*bx - ax*by; // if >0 then convex angle

            if(CV_SIGN( convexity ) == sign2 && (ax != 0 || ay != 0))
            {
                pprev = pcur;
                pcur = pnext;
                pnext += incr;
                stack[stacksize] = pnext;
                stacksize++;
            }
            else
            {
                if( pprev == start )
                {
                    pcur = pnext;
                    stack[1] = pcur;
                    pnext += incr;
                    stack[2] = pnext;
                }
                else
                {
                    stack[stacksize-2] = pnext;
                    pcur = pprev;
                    pprev = stack[stacksize-4];
                    stacksize--;
                }
            }
        }
        else
        {
            pnext += incr;
            stack[stacksize-1] = pnext;
        }
    }

    return --stacksize;
}

int comparator(void* pt1, void* pt2)
{
    Point* p1 = (Point*)pt1;
    Point* p2 = (Point*)pt2;
    if(p1->x < p2->x)
        return -1;

    if(p1->x == p2->x && p1->y < p2->y)
        return -1;

    return 1;
}

void convexHull(Mat points, vector* _hull, bool clockwise /* false */)
{
    int i, total = checkVector(points, 2, -1), depth = depth(points), nout = 0;
    int miny_ind = 0, maxy_ind = 0;
    assert(total >= 0 && (depth == 5 || depth == 4));

    if(total == 0)
    {
        vector_free(_hull);
        return;
    }

    returnPoints = !_hull.fixedType() ? returnPoints : _hull.type() != 4;
    AutoBuffer _pointer = init_AutoBufferPoint_(total);
    AutoBuffer _stack = init_AutoBuffer(total + 2), _hullbuf = init_AutoBuffer(total);
    Point** pointer = _pointer.ptr;
    Point* data0 = (Point*)ptr(points, 0);
    int* stack = _stack.ptr;
    int* hullbuf = _hullbuf.ptr;

    assert(isContinuous(points));

    for(i = 0; i < total; i++)
        pointer[i] = &data0[i];

    // sort the point set by x-coordinate, find min and max y
    qsort(pointer, total, sizeof(Point*), comparator);
    for(i = 1; i < total; i++)
    {
        int y = pointer[i]->y;
        if(pointer[miny_ind]->y > y)
            miny_ind = i;
        if(pointer[maxy_ind]->y < y)
            maxy_ind = i;
    }
    
    if( pointer[0]->x == pointer[total-1]->x &&
        pointer[0]->y == pointer[total-1]->y )
    {
        hullbuf[nout++] = 0;
    }
    else
    {
        // upper half
        int *tl_stack = stack;
        int tl_count = Sklansky_( pointer, 0, maxy_ind, tl_stack, -1, 1);
        int *tr_stack = stack + tl_count;
        int tr_count = Sklansky_( pointer, total-1, maxy_ind, tr_stack, -1, -1) :

        // gather upper part of convex hull to output
        if(!clockwise)
        {
            swap( tl_stack, tr_stack );
            swap( tl_count, tr_count );
        }

        for( i = 0; i < tl_count-1; i++ )
            hullbuf[nout++] = int(pointer[tl_stack[i]] - data0);
        for( i = tr_count - 1; i > 0; i-- )
            hullbuf[nout++] = int(pointer[tr_stack[i]] - data0);
        int stop_idx = tr_count > 2 ? tr_stack[1] : tl_count > 2 ? tl_stack[tl_count - 2] : -1;

        // lower half
        int *bl_stack = stack;
        int bl_count = Sklansky_( pointer, 0, miny_ind, bl_stack, 1, -1);
        int *br_stack = stack + bl_count;
        int br_count = Sklansky_( pointer, total-1, miny_ind, br_stack, 1, 1);

        if( clockwise )
        {
            swap( bl_stack, br_stack );
            swap( bl_count, br_count );
        }

        if( stop_idx >= 0 )
        {
            int check_idx = bl_count > 2 ? bl_stack[1] :
            bl_count + br_count > 2 ? br_stack[2-bl_count] : -1;
            if( check_idx == stop_idx || (check_idx >= 0 &&
                                          pointer[check_idx]->x == pointer[stop_idx]->x &&
                                          pointer[check_idx]->y == pointer[stop_idx]->y) )
            {
                // if all the points lie on the same line, then
                // the bottom part of the convex hull is the mirrored top part
                // (except the exteme points).
                bl_count = min( bl_count, 2 );
                br_count = min( br_count, 2 );
            }
        }

        for( i = 0; i < bl_count-1; i++ )
            hullbuf[nout++] = int(pointer[bl_stack[i]] - data0);
        for( i = br_count-1; i > 0; i-- )
            hullbuf[nout++] = int(pointer[br_stack[i]] - data0);
    }
    vector_resize(_hull, len);
    Mat hull = getMatfromVector(_hull);
    size_t step = isContinuous(hull) ? hull.step[0] : sizeof(Point);
    for(i = 0; i < nout; i++)
        *(Point*)vector_get(_hull, i*step) = data0[hullbuf[i]];
}

MatIterator matIterator(const Mat** _arrays, unsigned char** _ptrs, int _narrays)
{
    MatIterator it;
    int i, j, d1=0, i0 = -1, d = -1;

    it.arrays = _arrays;
    it.ptrs = _ptrs;
    it.narrays = _narrays;
    it.nplanes = 0;
    it.size = 0;

    it.iterdepth = 0;

    for(i = 0; i < it.narrays; i++)
    {
        Mat* A = it.arrays[i];
        if(it.ptrs)
            ptrs[i] = A->data;

        if(!A->data)
            continue;

        int sz[2] = {A->rows, A->cols};
        if(i0 < 0)
        {
            i0 = i;

            for(d1 = 0; d1 < 2; d1++)
            {
                if(sz[d1] > 1)
                    break;
            }

        }

        if(!isContinuous(*A))
        {
            for(j = 1; j > d1; j--)
            {
                if(A->step[j]*sz[j] < A->step[j-1])
                    break;
            }
            it.iterdepth = max(it.iterdepth, j);
        }
    }
    if(i0 >= 0)
    {
        int sz[2] = {arrays[i0]->rows, arrays[i0]->cols};
        it.size = sz[1];
        for(j = 1; j > it.iterdepth; j--)
        {
            int64_t total1 = (int64_t)it.size*sz[j-1];
            if(total1 != (int)total1)
                break;
            size = (int)total1;
        }

        it.iterdepth = j;
        if(it.iterdepth == d1)
            it.iterdepth = 0;

        it.nplanes = 0;
        for(j = it.iterdepth-1; j >= 0; j--)
            nplanes *= sz[j];
    }
    else
        it.iterdepth = 0;

    idx = 0;
}

bool validInterval(Diff8uC1 obj, const unsigned char* a, const unsigned char* b)
{
    return (unsigned)(a[0] - b[0] + obj.lo) <= obj.interval;
}

void cvt8u64f(const unsigned char* src, size_t sstep, double* dst, size_t dstep, Size size)
{
    sstep /= sizeof(src[0]);
    dstep /= sizeof(dst[0]);

    for(; size.height--; src += sstep, dst += dstep)
    {
        int x = 0;
        for(; x < size.width; x++)
            dst[x] = (double)src[x];
    }
}

void cvt64f8u(const double* src, size_t sstep, unsigned char* dst, size_t dstep, Size size)
{
    sstep /= sizeof(src[0]);
    dstep /= sizeof(dst[0]);

    for(; size.height--; src += sstep, dst += dstep)
    {
        int x = 0;
        for(; x < size.width; x++)
            dst[x] = (unsigned char)src[x];
    }
}

void convertAndUnrollScalar(const Mat sc, int buftype, unsigned char* scbuf, size_t blocksize, int code)
{
    int scn = sc.rows * sc.cols, cn = CV_MAT_CN(buftype);
    size_t esz = CV_ELEM_SIZE(buftype);
    if(code == 0)
        cvt8u64f(ptr(sc, 0), 1, scbuf, 1, init_size(min(cn, scn), 1));

    if(code == 1)
        cvt64f8u(ptr(sc, 0), 1, scbuf, 1, init_size(min(cn, scn), 1));
    // unroll the scalar
    if(scn < cn)
    {
        size_t esz1 = CV_ELEM_SIZE1(buftype);
        for(size_t i = esz1; i < esz; i++)
            scbuf[i] = scbuf[i - esz1];
    }
    for(size_t i = esz; i < blocksize*esz; i++)
        scbuf[i] = scbuf[i - esz];
}

void sub8u(const unsigned char* src1, size_t step1, const unsigned char* src2, size_t step2, unsigned char* dst, size_t step, int width, int height)
{
    for(; height--; src1 = (const unsigned char*)((const unsigned char*)src1 + step1),
                        src2 = (const unsigned char*)((const unsigned char*)src2 + step2),
                        dst = (unsigned char*)((unsigned char*)dst + step))
    {
        int x = 0;
        for(; x < width; x++)
            dst[x] = (unsigned char)(src1[x] + src2[x]);
    }
}

void subtract(const Mat src1, const Scalar psrc2, Mat* dst)
{
    int kind1 =  65536, kind2 = 131072;
    bool haveMask = false;
    int type1 = 0, depth1 = 0, cn = 1;
    int type2 = 6, depth2 = 6, cn2 = 1;
    int wtype;
    Size sz1 = init_size(src1.cols, src1.rows);
    Size sz2 = init_size(1, 4);

    bool haveScalar = true, swapped12 = truncatePrunedTree;
    depth2 = 0;

    int dtype = type1;

    dtype = 0;
    wtype = 0;

    create(dst, src1.rows, src1.cols, dtype);

    size_t esz1 = CV_ELEM_SIZE(type1), esz2 = CV_ELEM_SIZE(type2);
    size_t dsz = CV_ELEM_SIZE(dtype), wsz = CV_ELEM_SIZE(wtype);
    size_t blocksize0 = (size_t)(1024 + wsz-1)/wsz;
    Mat src2 = createMat(4, 1, -1056833530, &psrc2, 0);
    Mat mask = emptyMat();

    size_t bufesz = wsz;
    unsigned char *buf, *maskbuf = 0, *buf1 = 0, *buf2 = 0, *wbuf = 0;

    const Mat* arrays[] = { &src1, dst, &mask, 0 };
    unsigned char* ptrs[3];

    MatIterator it = matIterator(arrays, 0, ptrs, 3);
    size_t total = it.size, blocksize = min(total, blocksize0);

    AutoBuffer _buf = init_AutoBufferuchar(bufesz*blocksize + 64);
    buf = _buf.ptr;
    buf2 = buf;
    buf = alignPtr(buf + blocksize*wsz, 16);
    wbuf = maskbuf = buf;

    convertAndUnrollScalar(src2, wtype, buf2, blocksize, 0);

    for(size_t i = 0; i < it.nplanes; i++, getNextIterator(&it))
    {
        for(size_t j = 0; j < total; j += blocksize)
        {
            int bsz = min(total - j, blocksize);
            Size bszn = init_size(bsz*cn, 1);
            const unsigned char *sptr1 = ptrs[0];
            const unsigned char* sptr2 = buf2;
            unsigned char* dptr = ptrs[1];

            if(swapped12)
                swap_uchar(sptr1, sptr2);

            sub8u(sptr1, 1, sptr2, 1, dptr, 1, bszn.width, bszn.height, usrdata);
            ptrs[0] += bsz*esz1; ptrs[1] += bsz*dsz;
        }
    }
}

int borderInterpolate(int p, int len, int borderType)
{
    if((unsigned)p < (unsigned)len)
        ;
    else if(borderType == 1)
        p = p < 0 ? 0 : len - 1;
    else if(borderType == 2 || borderType == 4)
    {
        int delta = borderType == 4;
        if(len == 1)
            return 0;
        do
        {
            if(p < 0)
                p = -p-1 + delta;
            else
                p = len-1 - (p - len) - delta;
        }
        while((unsigned)p >= (unsigned)len);
    }
    else if( borderType == 3 )
    {
        assert(len > 0);
        if(p < 0)
            p -= ((p-len+1)/len)*len;
        if(p >= len)
            p %= len;
    }
    else if( borderType == 0 )
        p = -1;
    return p;
}

void copyMakeConstBorder_8u(unsigned char* src, size_t srcstep, Point srcroi,
                             unsigned char* dst, size_t dststep, Point dstroi,
                             int top, int left, int cn, unsigned char* value )
{
    int i, j;
    AutoBuffer* _constBuf = init_AutoBufferuchar(dstroi.width*cn);
    unsigned char* constBuf = _constBuf.ptr;
    int right = dstroi.x-srcroi.x-left;
    int bottom = dstroi.y-srcroi.y-top;

    for(i = 0; i < dstroi.x; i++)
    {
        for(j = 0; j < cn; j++)
            constBuf[i*cn+j] = value[j];
    }

    srcroi.x *= cn;
    dstroi.x *= cn;
    left *= cn;
    right *= cn;

    unsigned char* dstInner = dst + dststep*top + left;

    for(i = 0; i < srcroi.y; i++, dstInner += dststep, src += srcstep)
    {
        if(dstInner != src)
            memcpy(dstInner, src, srcroi.x);
        memcpy(dstInner-left, constBuf, left);
        memcpy(dstInner+srcroi.x, constBuf, right);
    }

    dst += dststep*top;

    for( i = 0; i < top; i++ )
        memcpy(dst+(i-top)*dststep, constBuf, dstroi.x);

    for( i = 0; i < bottom; i++ )
        memcpy(dst+(i+srcroi.y)*dststep, constBuf, dstroi.x);
}

void copyMakeBorder(Mat src, Mat* dst, int top, int bottom,
                         int left, int right, int borderType, Scalar value)
{
    assert(top >= 0 && bottom >= 0 && left >= 0 && right >= 0);
    int type = type(src);

    if(isSubmatrix(src) && && (borderType & 16) == 0)
    {
        Point *wholeSize = malloc(sizeof(Point));
        Point *ofs = malloc(sizeof(Point));
        locateROI(src, wholeSize, ofs);
        int dtop = min(ofs->y, top);
        int dbottom = min(wholeSize->y - src.rows - ofs->y, bottom);
        int dleft = min(ofs->x, left);
        int dright = min(wholeSize->x - src.cols - ofs->x, right);
        adjustROI(&src, dtop, dbottom, dleft, dright);
        top -= dtop;
        left -= dleft;
        bottom -= dbottom;
        right -= dright;
    }
    create(dst, src.rows + top + bottom, src.cols + left + right, type);

    if(top == 0 && left == 0 && bottom == 0 && right == 0)
    {
        if(src.data != dst->data || src.step != dst->step)
            copyTo(src, dst);
        return;
    }

    borderType &= ~16;

    int cn = channels(src), cn1 = cn;
    AutoBuffer buf = init_AutoBufferdouble(cn);
    if(cn > 4)
    {
        assert(value.val[0] == value.val[1] && value.val[0] == value.val[2] && value.val[0] == value.val[3]);
        cn1 = 1;
    }
    scalarToRawData(value, buf.ptr, CV_MAKETYPE(depth(src), cn1), cn);
    copyMakeConstBorder_8u(ptr(src, 0), src.step[0], init_Point(srcroi.cols, srcroi.rows),
                            ptr(*dst, 0), dst.step[0], init_Point(dst->cols, dst->rows),
                            top, left, (int)elemSize(src), buf);
}

void copymask_8u(const unsigned char* _src, size_t sstep, const unsigned char* mask, size_t mstep, unsigned char* _dst, size_t dstep, Size size)
{
    for(; size.height--; mask += mstep, _src += sstep, _dst += dstep)
    {
        const unsigned char* src = (const unsigned char*)_src;
        unsigned char* dst = (unsigned char*)_dst;
        int x = 0;
        for(; x < size.width; x++)
        {
            if(mask[x])
                dst[x] = src[x];
        }
    }
}

void setTo(Mat* m, Scalar s)
{
    int cn = channels(*m);
    size_t esz = elemSize(*m);

    Mat value = createMat(4, 1, -1056833530, &s, 0);
    const Mat* arrays[] = { m, 0, 0 };
    unsigned char* ptrs[2]={0,0};
    MatIterator it = matIterator(arrays, ptrs, 1);
    int totalsz = (int)it.size*mcn;
    int blockSize0 = min(totalsz, (int)((1024 + esz-1)/esz));
    blockSize0 -= blockSize0 % mcn;    // must be divisible without remainder for unrolling and advancing;
    AutoBuffer _scbuf = init_AutoBufferuchar(blockSize0*esz + 32);
    unsigned char* scbuf = alignptr((unsigned char*)_scbuf.data(), (int)sizeof(double));
    convertAndUnrollScalar(value, type(*m), scbuf, blockSize0/mcn, 1);

    for(size_t i = 0; i < it.nplanes; i++, getNextIterator(&it))
    {
        for(int j = 0; j < totalsz; j += blockSize0)
        {
            Size sz = init_size(min(blockSize0, totalsz - j), 1);
            size_t blockSize = sz.width*esz;
            if(ptrs[1])
            {
                copymask_8u(scbuf, 0, ptrs[1], 0, ptrs[0], 0, sz);
                ptrs[1] += sz.width;
            }
            else
                memcpy(ptrs[0], scbuf, blockSize);
            ptrs[0] += blockSize;
        }
    }
}

Mat* GetMat(const void* array, Mat* mat, int* pCOI, int allowND)
{
    Mat* result = 0;
    Mat* src = (Mat*)array;

    if(!mat || !src)
        fatal("NULL array pointer is passed");

    if(!src->data)
        fatal("The matrix has NULL data pointer");

    result = (Mat*)src;

    if( pCOI )
        *pCOI = 0;

    return result;
}

Mat* cvReshape(const void* array, Mat* header,
           int new_cn, int new_rows)
{
    Mat* result = 0;
    Mat *mat = (Mat*)array;
    int total_width, new_width;

    if(!header)
        fatal("NULL Pointer Error");

    if(!CV_IS_MAT(mat))
    {
        int coi = 0;
        mat = GetMat(mat, header, &coi, 1);
        if(coi)
            fatal("COI is not supported");
    }

    if( new_cn == 0 )
        new_cn = CV_MAT_CN(mat->type);
    else if((unsigned)(new_cn - 1) > 3)
        fatal("Bad number of channels");

    if(mat != header)
    {
        int hdr_refcount = header->hdr_refcount;
        *header = *mat;
        header->refcount = 0;
        header->hdr_refcount = hdr_refcount;
    }

    total_width = mat->cols * CV_MAT_CN(mat->type);

    if((new_cn > total_width || total_width % new_cn != 0) && new_rows == 0 )
        new_rows = mat->rows * total_width / new_cn;

    if(new_rows == 0 || new_rows == mat->rows)
    {
        header->rows = mat->rows;
        header->step = mat->step;
    }
    else
    {
        int total_size = total_width * mat->rows;
        if( !CV_IS_MAT_CONT(mat->type))
            fatal("The matrix is not continuous, thus its number of rows can not be changed");

        if((unsigned)new_rows > (unsigned)total_size)
            fatal("Bad new number of rows");

        total_width = total_size / new_rows;

        if(total_width * new_rows != total_size)
            fatal("The total number of matrix elements "
                                    "is not divisible by the new number of rows");

        header->rows = new_rows;
        header->step = total_width * CV_ELEM_SIZE1(mat->type);
    }

    new_width = total_width / new_cn;

    if(new_width * new_cn != total_width)
        fatal("The total width is not divisible by the new number of channels" );

    header->cols = new_width;
    header->type = (mat->type  & ~CV_MAT_TYPE_MASK) | CV_MAKETYPE(mat->type, new_cn);

    result = header;
    return  result;
}

// Calculates bounding rectagnle of a point set or retrieves already calculated
static Rect pointSetBoundingRect(Mat points)
{
    int npoints = checkVector(points, 2, -1);
    int depth = depth(points);
    assert(npoints >= 0 && (depth == CV_32F || depth == CV_32S));

    int  xmin = 0, ymin = 0, xmax = -1, ymax = -1, i;
    bool is_float = depth == CV_32F;

    if(npoints == 0)
        return init_Rect(0, 0, 0, 0);

    const Point* pts = (Point*)ptr(points, 0);
    Point pt = pts[0];

    if(!is_float)
    {
        xmin = xmax = pt.x;
        ymin = ymax = pt.y;

        for(i = 1; i < npoints; i++)
        {
            pt = pts[i];

            if(xmin > pt.x)
                xmin = pt.x;

            if(xmax < pt.x)
                xmax = pt.x;

            if(ymin > pt.y)
                ymin = pt.y;

            if(ymax < pt.y)
                ymax = pt.y;
        }
    }
    else
    {

            Cv32suf v;
            // init values
            xmin = xmax = CV_TOGGLE_FLT(pt.x);
            ymin = ymax = CV_TOGGLE_FLT(pt.y);

            for(i = 1; i < npoints; i++)
            {
                pt = pts[i];
                pt.x = CV_TOGGLE_FLT(pt.x);
                pt.y = CV_TOGGLE_FLT(pt.y);

                if(xmin > pt.x)
                    xmin = pt.x;

                if(xmax < pt.x)
                    xmax = pt.x;

                if(ymin > pt.y)
                    ymin = pt.y;

                if(ymax < pt.y)
                    ymax = pt.y;
            }

            v.i = CV_TOGGLE_FLT(xmin); xmin = Floor(v.f);
            v.i = CV_TOGGLE_FLT(ymin); ymin = Floor(v.f);
            // because right and bottom sides of the bounding rectangle are not inclusive
            // (note +1 in width and height calculation below), cvFloor is used here instead of cvCeil
            v.i = CV_TOGGLE_FLT(xmax); xmax = Floor(v.f);
            v.i = CV_TOGGLE_FLT(ymax); ymax = Floor(v.f);
    }

    return init_Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);
}

Rect boundingRect(vector* v)
{
    Mat m = getMatfromVector(v);
    return pointSetBoundingRect(m);
}

void reshape(Mat* m, int new_cn, int new_rows)
{
    int cn = channels(*m);

    if(new_cn == 0)
        new_cn = cn;

    int total_width = m->cols * cn;

    if((new_cn > total_width || total_width % new_cn != 0) && new_rows == 0)
        new_rows = m->rows * total_width/new_cn;

    if(new_rows != 0 && new_rows != rows)
    {
        int total_size = total_width * m.rows;
        if(!isContinuous(*m))
            fatal("The matrix is not continuous, thus its number of rows can not be changed");

        if((unsigned)new_rows > (unsigned)total_size)
            fatal("Bad new number of rows");

        total_width = total_size / new_rows;

        if(total_width * new_rows != total_size)
            fatal("The total number of matrix elements is not divisible by the new number of rows");

        m->rows = new_rows;
        m->step[0] = total_width * elemSize1(*m);
    }

    int new_width = total_width / new_cn;

    if(new_width * new_cn != total_width)
        fatal("The total width is not divisible by the new number of channels");

    m->cols = new_width;
    m->flags = (m->flags & ~4088) | ((new_cn-1) << 3);
    m->step[1] = CV_ELEM_SIZE(m->flags);
}

void copyToVector(Mat src, vector* dst)
{
    int dtype = 0;
    
    
    if(empty(src))
    {
        vector_free(dst);
        return;
    }

    size_t len = src.rows*src.cols > 0 ? src.rows + src.cols - 1 : 0;
    vector_resize(dst, len);
    Mat _dst = getMatfromVector(dst);
    
    if(src.data == _dst.data)
        return;

    if(src.rows > 0 && src.cols > 0)
    {
        reshape(&_dst, 0, total(_dst));

        const unsigned char* sptr = src.data;
        unsigned char* dptr = _dst.data;

        Point sz = getContinuousSize(src, _dst);
        size_t len = sz.x * elemSize(src);

        for(; sz.y--; sptr += src.step, dptr += dst.step)
            memcpy(dptr, sptr, len);
    }
}

static int approxPolyDP_(const Point* src_contour, int count0, Point* dst_contour, bool is_closed0, double eps, AutoBuffer* _stack)
{
    #define PUSH_SLICE(slice) \
        if(top >= stacksz) \
        { \
            AutoBuffer_resize(_stack, stacksz*3/2); \
            stack = *_stack; \
            stacksz = _stack->sz; \
        } \
        stack[top++] = slice

    #define READ_PT(pt, pos) \
        pt = src_contour[pos]; \
        if(++pos >= count) pos = 0

    #define READ_DST_PT(pt, pos) \
        pt = dst_contour[pos]; \
        if(++pos >= count) pos = 0

    #define WRITE_PT(pt) \
        dst_contour[new_count++] = pt

    int             init_iters = 3;
    Point slice = init_Point(0, 0); /* Range */
    Point right_slice = init_Point(0, 0); /* Range */
    Point start_pt = init_Point((int)-1000000, (int)-1000000); 
    Point end_pt = init_Point(0, 0);
    Point pt = init_Point(0, 0);
    int i = 0, j, pos = 0, wpos, count = count0, new_count=0;
    int is_closed = is_closed0;
    bool le_eps = false;
    size_t top = 0, stacksz = _stack->sz;
    Point* stack = (Point)_stack.ptr; /* Range */

    if(count == 0)
        return 0;

    eps *= eps;

    if(!is_closed)
    {
        right_slice.start = count;
        end_pt = src_contour[0];
        start_pt = src_contour[count-1];

        if(start_pt.x != end_pt.x || start_pt.y != end_pt.y)
        {
            slice.start = 0;
            slice.end = count - 1;
            PUSH_SLICE(slice);
        }
        else
        {
            is_closed = 1;
            init_iters = 1;
        }
    }

    if(is_closed)
    {
        // 1. Find approximately two farthest points of the contour
        right_slice.start = 0;

        for(i = 0; i < init_iters; i++)
        {
            double dist, max_dist = 0;
            pos = (pos + right_slice.start)%count;
            READ_PT(start_pt, pos);

            for(j = 1; j < count; j++)
            {
                double dx, dy;

                READ_PT(pt, pos);
                dx = pt.x - start_pt.x;
                dy = pt.y - start_pt.y;

                dist = dx * dx + dy * dy;

                if(dist > max_dist)
                {
                    max_dist = dist;
                    right_slice.start = j;
                }
            }

            le_eps = max_dist <= eps;
        }

        // 2. initialize the stack
        if(!le_eps)
        {
            right_slice.end = slice.start = pos % count;
            slice.end = right_slice.start = (right_slice.start + slice.start) % count;

            PUSH_SLICE(right_slice);
            PUSH_SLICE(slice);
        }
        else
            WRITE_PT(start_pt);
    }

    // 3. run recursive process
    while(top > 0)
    {
        slice = stack[--top];
        end_pt = src_contour[slice.end];
        pos = slice.start;
        READ_PT(start_pt, pos);

        if(pos != slice.end)
        {
            double dx, dy, dist, max_dist = 0;

            dx = end_pt.x - start_pt.x;
            dy = end_pt.y - start_pt.y;

            assert(dx != 0 || dy != 0);

            while(pos != slice.end)
            {
                READ_PT(pt, pos);
                dist = fabs((pt.y - start_pt.y) * dx - (pt.x - start_pt.x) * dy);

                if(dist > max_dist)
                {
                    max_dist = dist;
                    right_slice.start = (pos+count-1)%count;
                }
            }

            le_eps = max_dist * max_dist <= eps * (dx * dx + dy * dy);
        }
        else
        {
            le_eps = true;
            // read starting point
            start_pt = src_contour[slice.start];
        }

        if(le_eps)
        {
            WRITE_PT(start_pt);
        }
        else
        {
            right_slice.end = slice.end;
            slice.end = right_slice.start;
            PUSH_SLICE(right_slice);
            PUSH_SLICE(slice);
        }
    }

    if(!is_closed)
        WRITE_PT(src_contour[count-1]);

    // last stage: do final clean-up of the approximated contour -
    // remove extra points on the [almost] stright lines.
    is_closed = is_closed0;
    count = new_count;
    pos = is_closed ? count - 1 : 0;
    READ_DST_PT(start_pt, pos);
    wpos = pos;
    READ_DST_PT(pt, pos);

    for(i = !is_closed; i < count - !is_closed && new_count > 2; i++)
    {
        double dx, dy, dist, successive_inner_product;
        READ_DST_PT(end_pt, pos);

        dx = end_pt.x - start_pt.x;
        dy = end_pt.y - start_pt.y;
        dist = fabs((pt.x - start_pt.x)*dy - (pt.y - start_pt.y)*dx);
        successive_inner_product = (pt.x - start_pt.x) * (end_pt.x - pt.x) +
        (pt.y - start_pt.y) * (end_pt.y - pt.y);

        if(dist * dist <= 0.5*eps*(dx*dx + dy*dy) && dx != 0 && dy != 0 &&
           successive_inner_product >= 0)
        {
            new_count--;
            dst_contour[wpos] = start_pt = end_pt;
            if(++wpos >= count) wpos = 0;
            READ_DST_PT(pt, pos);
            i++;
            continue;
        }
        dst_contour[wpos] = start_pt = pt;
        if(++wpos >= count) wpos = 0;
        pt = end_pt;
    }

    if( !is_closed )
        dst_contour[wpos] = pt;

    return new_count;
}

void approxPolyDP(Mat curve, vector* approxCurve /* Point */, double epsilon, bool closed)
{
    int npoints = checkVector(curve, 2, -1), depth = depth(curve);
    assert(npoints >= 0 && (depth == CV_32S || depth == CV_32F));

    if(npoints == 0)
    {
        vector_free(approxCurve);
        return;
    }

    AutoBuffer _buf = init_AutoBufferPoint(npoints);
    AutoBuffer _stack = init_AutoBufferPoint(npoints);
    Point* buf = (Point*)_buf.ptr;
    int nout = 0;

    nout = approxPolyDP_((Point*)ptr(curve, 0), npoints, buf, closed, epsilon, _stack);

    copyToVector(createMat(nout, 1, CV_MAKETYPE(depth, 2), buf, 0), approxCurve);
}