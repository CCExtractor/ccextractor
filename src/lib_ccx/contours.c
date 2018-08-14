#include "math.h"
#include "Mat.h"
#include "types.h"

ThresholdRunner thresholdRunner(Mat _src, Mat _dst, double _thresh, double _maxval, int _thresholdType)
{
    ThresholdRunner tr;
    tr.src = _src;
    tr.dst = _dst;
    ts.thresh = _thresh;
    tr.maxval = _maxval;
    tr.thresholdType = _thresholdType;
    return tr;
}

int cvSliceLength(Point slice, const Seq* seq)
{
    int total = seq->total;
    int length = slice.y - slice.x;

    if(length != 0)
    {
        if( slice.x < 0)
            slice.x += total;
        if( slice.y <= 0 )
            slice.y += total;

        length = slice.y - slice.x;
    }

    while(length < 0)
        length += total;
    if(length > total)
        length = total;

    return length;
}

void cvInsertNodeIntoTree( Seq* _node, Seq* _parent, Seq* _frame)
{
    TreeNode* node = (TreeNode*)_node;
    TreeNode* parent = (TreeNode*)_parent;

    if( !node || !parent )
        fatal("NULL Pointer error");

    node->v_prev = _parent != _frame ? parent : 0;
    node->h_next = parent->v_next;

    assert(parent->v_next != node);

    if(parent->v_next)
        parent->v_next->h_prev = node;
    parent->v_next = node;
}

/* Copy all sequence elements into single continuous array: */
void* CvtSeqToArray(const Seq *seq, void *array, Point slice)
{
    int elem_size, total;
    SeqReader reader;
    char *dst = (char*)array;

    elem_size = seq->elem_size;
    total = cvSliceLength(slice, seq)*elem_size;

    if(total == 0)
        return 0;

    StartReadSeq(seq, &reader, 0);
    SetSeqReaderPos(&reader, slice.start_index, 0);

    do
    {
        int count = (int)(reader.block_max - reader.ptr);
        if( count > total )
            count = total;

        memcpy( dst, reader.ptr, count );
        dst += count;
        reader.block = reader.block->next;
        reader.ptr = reader.block->data;
        reader.block_max = reader.ptr + reader.block->count*elem_size;
        total -= count;
    }
    while(total > 0);

    return array;
}

// area of a whole sequence
double contourArea(vector* _contour/* Point */, bool oriented)
{
    Mat contour = getMatfromVector(_contour);
    int npoints = checkVector(contour, 2, -1);
    int depth = depth(contour);
    assert(npoints >= 0 && (depth == CV_32F || depth == CV_32S));

    if(npoints == 0)
        return 0.;

    double a00 = 0;
    bool is_float = depth == 5;
    const floatPoint* ptsi = (Point*)ptr(contour, 0);
    const floatPoint* ptsf = (floatPoint*)ptr(contour, 0);
    floatPoint prev = is_float ? ptsf[npoints-1] : init_floatPoint((float)ptsi[npoints-1].x, (float)ptsi[npoints-1].y);

    for(int i = 0; i < npoints; i++)
    {
        floatPoint p = is_float ? ptsf[i] : init_floatPoint((float)ptsi[i].x, (float)ptsi[i].y);
        a00 += (double)prev.x * p.y - (double)prev.y * p.x;
        prev = p;
    }

    a00 *= 0.5;
    if(!oriented)
        a00 = fabs(a00);

    return a00;
}

/* curvature: 0 - 1-curvature, 1 - k-cosine curvature. */
Seq* icvApproximateChainTC89(Chain* chain, int header_size, MemStorage* storage, int method)
{
    static const int abs_diff[] = { 1, 2, 3, 4, 3, 2, 1, 0, 1, 2, 3, 4, 3, 2, 1 };

    PtInfo       temp;
    PtInfo       *array = malloc(sizeof(PtInfo)* (chain->total + 8)), *first = 0, *current = 0, *prev_current = 0;
    int             i, j, i1, i2, s, len;
    int             count = chain->total;

    ChainPtReader reader;
    SeqWriter     writer;
    Point         pt = chain->origin;

    assert(CV_IS_SEQ_CHAIN_CONTOUR(chain));
    assert(header_size >= (int)sizeof(Contour));

    StartWriteSeq((chain->flags & ~16383) | CV_MAKETYPE(4,2),
                     header_size, sizeof(Point), storage, &writer);

    if(chain->total == 0)
    {
        CV_WRITE_SEQ_ELEM( pt, writer );
        return EndWriteSeq( &writer );
    }

    reader.code = 0;
    StartReadChainPoints(chain, &reader);

    temp.next = 0;
    current = &temp;

    /* Pass 0.
       Restores all the digital curve points from the chain code.
       Removes the points (from the resultant polygon)
       that have zero 1-curvature */
    for(i = 0; i < count; i++)
    {
        int prev_code = *reader.prev_elem;

        reader.prev_elem = reader.ptr;
        CV_READ_CHAIN_POINT(pt, reader);

        /* calc 1-curvature */
        s = abs_diff[reader.code - prev_code + 7];

        if(method <= 2)
        {
            if(method == CV_CHAIN_APPROX_NONE || s != 0)
            {
                CV_WRITE_SEQ_ELEM(pt, writer);
            }
        }
        else
        {
            if( s != 0 )
                current = current->next = array + i;
            array[i].s = s;
            array[i].pt = pt;
        }
    }

    if(method <= 2)
        return EndWriteSeq(&writer);

    current->next = 0;

    len = i;
    current = temp.next;

    assert(current);

    /* Pass 1.
       Determines support region for all the remained points */
    do
    {
        Point pt0;
        int k, l = 0, d_num = 0;

        i = (int)(current - array);
        pt0 = array[i].pt;

        /* determine support region */
        for(k = 1;; k++)
        {
            int lk, dk_num;
            int dx, dy;
            Cv32suf d;

            assert(k <= len);

            /* calc indices */
            i1 = i - k;
            i1 += i1 < 0 ? len : 0;
            i2 = i + k;
            i2 -= i2 >= len ? len : 0;

            dx = array[i2].pt.x - array[i1].pt.x;
            dy = array[i2].pt.y - array[i1].pt.y;

            /* distance between p_(i - k) and p_(i + k) */
            lk = dx * dx + dy * dy;

            /* distance between p_i and the line (p_(i-k), p_(i+k)) */
            dk_num = (pt0.x - array[i1].pt.x) * dy - (pt0.y - array[i1].pt.y) * dx;
            d.f = (float)(((double) d_num) * lk - ((double) dk_num) * l);

            if( k > 1 && (l >= lk || ((d_num > 0 && d.i <= 0) || (d_num < 0 && d.i >= 0))))
                break;

            d_num = dk_num;
            l = lk;
        }

        current->k = --k;

        /* determine cosine curvature if it should be used */
        if( method == 4 )
        {
            /* calc k-cosine curvature */
            for( j = k, s = 0; j > 0; j-- )
            {
                double temp_num;
                int dx1, dy1, dx2, dy2;
                Cv32suf sk;

                i1 = i - j;
                i1 += i1 < 0 ? len : 0;
                i2 = i + j;
                i2 -= i2 >= len ? len : 0;

                dx1 = array[i1].pt.x - pt0.x;
                dy1 = array[i1].pt.y - pt0.y;
                dx2 = array[i2].pt.x - pt0.x;
                dy2 = array[i2].pt.y - pt0.y;

                if( (dx1 | dy1) == 0 || (dx2 | dy2) == 0 )
                    break;

                temp_num = dx1 * dx2 + dy1 * dy2;
                temp_num =
                    (float) (temp_num /
                             sqrt(((double)dx1 * dx1 + (double)dy1 * dy1) *
                                   ((double)dx2 * dx2 + (double)dy2 * dy2)));
                sk.f = (float)(temp_num + 1.1);

                assert(0 <= sk.f && sk.f <= 2.2);
                if( j < k && sk.i <= s )
                    break;

                s = sk.i;
            }
            current->s = s;
        }
        current = current->next;
    }
    while(current != 0);

    prev_current = &temp;
    current = temp.next;

    /* Pass 2.
       Performs non-maxima suppression */
    do
    {
        int k2 = current->k >> 1;

        s = current->s;
        i = (int)(current - array);

        for(j = 1; j <= k2; j++)
        {
            i2 = i - j;
            i2 += i2 < 0 ? len : 0;

            if(array[i2].s > s)
                break;

            i2 = i + j;
            i2 -= i2 >= len ? len : 0;

            if(array[i2].s > s)
                break;
        }

        if(j <= k2)           /* exclude point */
        {
            prev_current->next = current->next;
            current->s = 0;     /* "clear" point */
        }
        else
            prev_current = current;
        current = current->next;
    }
    while(current != 0);

    /* Pass 3.
       Removes non-dominant points with 1-length support region */
    current = temp.next;
    assert(current);
    prev_current = &temp;

    do
    {
        if( current->k == 1 )
        {
            s = current->s;
            i = (int)(current - array);

            i1 = i - 1;
            i1 += i1 < 0 ? len : 0;

            i2 = i + 1;
            i2 -= i2 >= len ? len : 0;

            if( s <= array[i1].s || s <= array[i2].s )
            {
                prev_current->next = current->next;
                current->s = 0;
            }
            else
                prev_current = current;
        }
        else
            prev_current = current;
        current = current->next;
    }
    while( current != 0 );

    if( method == 4 )
        goto copy_vect;

    /* Pass 4.
       Cleans remained couples of points */

    assert( temp.next );

    if( array[0].s != 0 && array[len - 1].s != 0 )      /* specific case */
    {
        for( i1 = 1; i1 < len && array[i1].s != 0; i1++ )
        {
            array[i1 - 1].s = 0;
        }
        if( i1 == len )
            goto copy_vect;     /* all points survived */
        i1--;

        for( i2 = len - 2; i2 > 0 && array[i2].s != 0; i2-- )
        {
            array[i2].next = 0;
            array[i2 + 1].s = 0;
        }
        i2++;

        if( i1 == 0 && i2 == len - 1 )  /* only two points */
        {
            i1 = (int)(array[0].next - array);
            array[len] = array[0];      /* move to the end */
            array[len].next = 0;
            array[len - 1].next = array + len;
        }
        temp.next = array + i1;
    }

    current = temp.next;
    first = prev_current = &temp;
    count = 1;

     /* do last pass */
    do
    {
        if( current->next == 0 || current->next - current != 1 )
        {
            if( count >= 2 )
            {
                if( count == 2 )
                {
                    int s1 = prev_current->s;
                    int s2 = current->s;

                    if( s1 > s2 || (s1 == s2 && prev_current->k <= current->k) )
                        /* remove second */
                        prev_current->next = current->next;
                    else
                        /* remove first */
                        first->next = current;
                }
                else
                    first->next->next = current;
            }
            first = current;
            count = 1;
        }
        else
            count++;
        prev_current = current;
        current = current->next;
    }
    while(current != 0);

    copy_vect:

    // gather points
    current = temp.next;
    assert(current);

    do
    {
        CV_WRITE_SEQ_ELEM( current->pt, writer );
        current = current->next;
    }
    while(current != 0);

    return EndWriteSeq(&writer);
}

static void thresh_8u(const Mat src, Mat* dst, unsigned char thresh, unsigned char maxval, int type)
{
    int roi[2] = {src.rows, src.cols};
    roi[1] *= channels(src);
    size_t src_step = src.step;
    size_t dst_step = dst.step;

    if(isContinuous(src) && isContinuous(*dst))
    {
        roi[1] *= roi[0];
        roi[0] = 1;
        src_step = dst_step = roi[1];
    }
    int j = 0;
    const unsigned char* _src = ptr(src, 0);
    unsigned char* _dst = ptr(*dst, 0);

    int j_scalar = j;
    if(j_scalar < roi[1])
    {
        const int thresh_pivot = thresh + 1;
        unsigned char tab[256] = {0};
        
        memset(tab, 0, thresh_pivot);
        if (thresh_pivot < 256) 
            memset(tab + thresh_pivot, maxval, 256-thresh_pivot);

        _src = ptr(src, 0);
        _dst = ptr(*dst, 0);

        for(int i = 0; i < roi[0]; i++, _src += src_step, _dst += dst_step)
        {
            j = j_scalar;
            for(; j < roi[1]; j++)
                _dst[j] = tab[_src[j]];
        }
    }
}

void rangeop(ThresholdRunner* body, Point range)
{
    int row0 = range.x;
    int row1 = range.y;

    Mat srcStripe = createusingRange(body->src, row0, row1);
    Mat dstStripe = createusingRange(body->dst, row0, row1);

    thresh_8u(srcStripe, &dstStripe, (unsigned char)body->thresh, (unsigned char)body->maxval, body->thresholdType);
}

void parallel_for_(Point range, ThresholdRunner body, double nstripes)
{
    (void)nstripes;
    rangeop(&body, range);
}

double threshold(Mat* src, Mat* dst, double thresh, double maxval, int type)
{
    create(dst, src->rows, src->cols, type(src));
    int ithresh = floor(thresh);
    thresh = ithresh;
    int imaxval = round(maxval);
    imaxval = (unsigned char)imaxval;
    thresh = ithresh;
    maxval = imaxval;
    parallel_for_(init_Point(0, dst->rows),
                  thresholdRunner(src, dst, thresh, maxval, type),
                  total(*dst)/(double)(1<<16));
    return thresh
}

double cvThreshold(const Mat* src, Mat* dst, double thresh, double maxval, int type)
{
    Mat dst0 = *dst;

    assert(src->rows == dst->rows && src->cols == dst->cols && channels(*src) == channels(*dst) &&
        (depth(*src) == depth(*dst) || depth(dst0) == CV_8U));

    thresh = threshold(src, dst, thresh, maxval, type);
    return thresh;
}

static void icvFetchContourEx(signed char* ptr, int step, Point pt, Seq* contour, int  _method, int nbd, Rect* _rect)
{
    int deltas[16];
    SeqWriter writer;
    signed char *i0 = ptr, *i1, *i3, *i4 = NULL;
    Rect rect;
    int prev_s = -1, s, s_end;
    int method = _method - 1;

    /* initialize local state */
    CV_INIT_3X3_DELTAS( deltas, step, 1);
    memcpy(deltas + 8, deltas, 8 * sizeof(deltas[0]));

    /* initialize writer */
    StartAppendToSeq(contour, &writer);

    if(method < 0)
        ((Chain *)contour)->origin = pt;

    rect.x = rect.width = pt.x;
    rect.y = rect.height = pt.y;

    s_end = s = CV_IS_SEQ_HOLE( contour ) ? 0 : 4;

    do
    {
        s = (s - 1) & 7;
        i1 = i0 + deltas[s];
    }
    while(*i1 == 0 && s != s_end);

    if(s == s_end)            /* single pixel domain */
    {
        *i0 = (signed char)(nbd | 0x80);
        if(method >= 0)
        {
            CV_WRITE_SEQ_ELEM(pt, writer);
        }
    }
    else
    {
        i3 = i0;

        prev_s = s ^ 4;

        /* follow border */
        for(;;)
        {
            assert(i3 != NULL);
            s_end = s;
            s = min(s, 15);

            while(s < 15)
            {
                i4 = i3 + deltas[++s];
                assert(i4 != NULL);
                if(*i4 != 0)
                    break;
            }
            s &= 7;

            /* check "right" bound */
            if((unsigned)(s-1) < (unsigned) s_end)
            {
                *i3 = (signed char)(nbd | 0x80);
            }
            else if(*i3 == 1)
            {
                *i3 = (signed char) nbd;
            }

            if(method < 0)
            {
                signed char _s = (signed char) s;
                CV_WRITE_SEQ_ELEM(_s, writer);
            }
            else if( s != prev_s || method == 0 )
            {
                CV_WRITE_SEQ_ELEM( pt, writer );
            }

            if(s != prev_s)
            {
                /* update bounds */
                if(pt.x < rect.x)
                    rect.x = pt.x;
                else if(pt.x > rect.width)
                    rect.width = pt.x;

                if(pt.y < rect.y)
                    rect.y = pt.y;
                else if(pt.y > rect.height)
                    rect.height = pt.y;
            }

            prev_s = s;
            pt.x += icvCodeDeltas[s].x;
            pt.y += icvCodeDeltas[s].y;

            if(i4 == i0 && i3 == i1)  break;

            i3 = i4;
            s = (s+4) & 7;
        }                       /* end of border following loop */
    }

    rect.width -= rect.x - 1;
    rect.height -= rect.y - 1;

    EndWriteSeq(&writer);

    if(_method != CV_CHAIN_CODE)
        ((Contour*)contour)->rect = rect;

    if(_rect)
    	*_rect = rect;
}

/*
   trace contour until certain point is met.
   returns 1 if met, 0 else.
*/
static int icvTraceContour(signed char *ptr, int step, signed char *stop_ptr, int is_hole)
{
    int deltas[16];
    signed char *i0 = ptr, *i1, *i3, *i4 = NULL;
    int s, s_end;

    /* initialize local state */
    CV_INIT_3X3_DELTAS( deltas, step, 1 );
    memcpy(deltas + 8, deltas, 8 * sizeof(deltas[0]));

    s_end = s = is_hole ? 0 : 4;

    do
    {
        s = (s - 1) & 7;
        i1 = i0 + deltas[s];
    }
    while(*i1 == 0 && s != s_end);

    i3 = i0;

    /* check single pixel domain */
    if(s != s_end)
    {
        /* follow border */
        for(;;)
        {
            assert(i3 != NULL);

            s = min(s, 15);
            while( s < 15 )
            {
                i4 = i3 + deltas[++s];
                assert(i4 != NULL);
                if(*i4 != 0)
                    break;
            }

            if(i3 == stop_ptr || (i4 == i0 && i3 == i1))
                break;

            i3 = i4;
            s = (s + 4) & 7;
        }                       /* end of border following loop */
    }
    return i3 == stop_ptr;
}

static int icvTraceContour_32s(int *ptr, int step, int *stop_ptr, int is_hole)
{
    assert(ptr != NULL);
    int deltas[16];
    int *i0 = ptr, *i1, *i3, *i4 = NULL;
    int s, s_end;
    const int   right_flag = INT_MIN;
    const int   new_flag = (int)((unsigned)INT_MIN >> 1);
    const int   value_mask = ~(right_flag | new_flag);
    const int   ccomp_val = *i0 & value_mask;

    /* initialize local state */
    CV_INIT_3X3_DELTAS(deltas, step, 1);
    memcpy(deltas + 8, deltas, 8 * sizeof( deltas[0]));

    s_end = s = is_hole ? 0 : 4;

    do
    {
        s = (s - 1) & 7;
        i1 = i0 + deltas[s];
    }
    while((*i1 & value_mask) != ccomp_val && s != s_end);

    i3 = i0;

    /* check single pixel domain */
    if(s != s_end)
    {
        /* follow border */
        for(;;)
        {
            assert(i3 != NULL);
            s_end = s;
            s = min(s, 15);

            while(s < 15)
            {
                i4 = i3 + deltas[++s];
                assert(i4 != NULL);
                if((*i4 & value_mask) == ccomp_val)
                    break;
            }

            if(i3 == stop_ptr || (i4 == i0 && i3 == i1))
                break;

            i3 = i4;
            s = (s + 4) & 7;
        }                       /* end of border following loop */
    }
    return i3 == stop_ptr;
}

static void icvFetchContourEx_32s(int* ptr, int step, Point pt, Seq* contour, int _method, Rect* _rect)
{
    assert(ptr != NULL);
    int deltas[16];
    SeqWriter writer;
    int *i0 = ptr, *i1, *i3, *i4;
    Rect rect;
    int prev_s = -1, s, s_end;
    int method = _method-1;
    const int right_flag = INT_MIN;
    const int new_flag = (int)((unsigned)INT_MIN >> 1);
    const int value_mask = ~(right_flag | new_flag);
    const int ccomp_val = *i0 & value_mask;
    const int nbd0 = ccomp_val | new_flag;
    const int nbd1 = nbd0 | right_flag;

    /* initialize local state */
    CV_INIT_3X3_DELTAS(deltas, step, 1);
    memcpy(deltas + 8, deltas, 8 * sizeof(deltas[0]));

    /* initialize writer */
    StartAppendToSeq(contour, &writer);

    if(method < 0)
        ((Chain*)contour)->origin = pt;

    rect.x = rect.width = pt.x;
    rect.y = rect.height = pt.y;

    s_end = s = CV_IS_SEQ_HOLE(contour) ? 0 : 4;

    do
    {
        s = (s - 1) & 7;
        i1 = i0 + deltas[s];
    }
    while((*i1 & value_mask) != ccomp_val && s != s_end && (s < 15));

    if(s == s_end)            /* single pixel domain */
    {
        *i0 = nbd1;
        if(method >= 0)
        {
            CV_WRITE_SEQ_ELEM(pt, writer);
        }
    }
    else
    {
        i3 = i0;
        prev_s = s ^ 4;

        /* follow border */
        for(;;)
        {
            assert(i3 != NULL);
            s_end = s;

            do
            {
                i4 = i3 + deltas[++s];
                assert(i4 != NULL);
            }
            while((*i4 & value_mask) != ccomp_val && (s < 15));
            s &= 7;

            /* check "right" bound */
            if((unsigned)(s-1) < (unsigned) s_end)
            {
                *i3 = nbd1;
            }
            else if(*i3 == ccomp_val)
            {
                *i3 = nbd0;
            }

            if(method < 0)
            {
                signed char _s = (signed char)s;
                CV_WRITE_SEQ_ELEM( _s, writer );
            }
            else if(s != prev_s || method == 0)
            {
                CV_WRITE_SEQ_ELEM(pt, writer);
            }

            if(s != prev_s)
            {
                /* update bounds */
                if(pt.x < rect.x)
                    rect.x = pt.x;
                else if(pt.x > rect.width)
                    rect.width = pt.x;

                if(pt.y < rect.y)
                    rect.y = pt.y;
                else if(pt.y > rect.height)
                    rect.height = pt.y;
            }

            prev_s = s;
            pt.x += icvCodeDeltas[s].x;
            pt.y += icvCodeDeltas[s].y;

            if( i4 == i0 && i3 == i1 )  break;

            i3 = i4;
            s = (s + 4) & 7;
        }                       /* end of border following loop */
    }

    rect.width -= rect.x - 1;
    rect.height -= rect.y - 1;

    EndWriteSeq(&writer);

    if(_method != CV_CHAIN_CODE)
        ((Contour*)contour)->rect = rect;

    if( _rect )  *_rect = rect;
}

Seq* FindNextContour(ContourScanner* scanner)
{
    if(!scanner)
        fatal("NULL Point error");

    assert(scanner->img_step >= 0);

    icvEndProcessContour(scanner);

    /* initialize local state */
    signed char* img0 = scanner->img0;
    signed char* img = scanner->img;
    int step = scanner->img_step;
    int step_i = step / sizeof(int);
    int x = scanner->pt.x;
    int y = scanner->pt.y;
    int width = scanner->img_size.x;
    int height = scanner->img_size.y;
    int mode = scanner->mode;
    Point lnbd = scanner->lnbd;
    int nbd = scanner->nbd;
    int prev = img[x - 1];
    int new_mask = -2;

    if(mode == 4)
    {
        prev = ((int*)img)[x - 1];
        new_mask = INT_MIN / 2;
    }

    for(; y < height; y++, img += step)
    {
        int* img0_i = 0;
        int* img_i = 0;
        int p = 0;

        if(mode == 4)
        {
            img0_i = (int*)img0;
            img_i = (int*)img;
        }

        for(; x < width; x++)
        {
            if(img_i)
            {
                for(; x < width && ((p = img_i[x]) == prev || (p & ~new_mask) == (prev & ~new_mask)); x++ )
                    prev = p;
            }
            else
            {
                for(; x < width && (p = img[x]) == prev; x++)
                    ;
            }

            if(x >= width)
                break;

            ContourInfo *par_info = 0;
            ContourInfo *l_cinfo = 0;
            Seq *seq = 0;
            int is_hole = 0;
            Point origin;

             /* if not external contour */
            if( (!img_i && !(prev == 0 && prev == 1)) || (img_i && !(((prev & new_mask) != 0 || prev == 0) && (p & new_mask) == 0)))
            {
                /* check hole */
                if( (!img_i && (p != 0 || prev < 1)) ||
                        (img_i && ((prev & new_mask) != 0 || (p & new_mask) != 0)))
                    goto resume_scan;

                if(prev & new_mask)
                    lnbd.x = x-1;

                is_hole = 1;
            }

            if(mode == 0 && (is_hole || img0[lnbd.y * (size_t)(step) + lnbd.x] > 0))
                goto resume_scan;

            origin.y = y;
            origin.x = x - is_hole;

            /* find contour parent */
            if(mode <= 1 || (!is_hole && (mode == CV_RETR_CCOMP || mode == CV_RETR_FLOODFILL)) || lnbd.x <= 0)
                par_info = &(scanner->frame_info);

            else
            {
                int lval = (img0_i ?
                        img0_i[lnbd.y * (size_t)(step_i) + lnbd.x] :
                        (int)img0[lnbd.y * (size_t)(step) + lnbd.x]) & 0x7f;
                ContourInfo *cur = scanner->cinfo_table[lval];

                /* find the first bounding contour */
                while(cur)
                {
                    if((unsigned) (lnbd.x - cur->rect.x) < (unsigned) cur->rect.width &&
                            (unsigned) (lnbd.y - cur->rect.y) < (unsigned) cur->rect.height)
                    {
                        if(par_info)
                        {
                            if((img0_i &&
                                     icvTraceContour_32s(img0_i + par_info->origin.y * (size_t)(step_i) +
                                                          par_info->origin.x, step_i, img_i + lnbd.x,
                                                          par_info->is_hole) > 0) ||
                                    (!img0_i &&
                                     icvTraceContour(img0 + par_info->origin.y * (size_t)(step) +
                                                      par_info->origin.x, step, img + lnbd.x,
                                                      par_info->is_hole) > 0))
                                break;
                        }
                        par_info = cur;

                    }
                    cur = cur->next;
                }
                assert(par_info != 0)

                 /* if current contour is a hole and previous contour is a hole or
                       current contour is external and previous contour is external then
                       the parent of the contour is the parent of the previous contour else
                       the parent is the previous contour itself. */
                if(par_info->is_hole == is_hole)
                {
                    par_info = par_info->parent;
                    /* every contour must have a parent
                       (at least, the frame of the image) */
                    if(!par_info)
                        par_info = &(scanner->frame_info);
                }

                /* hole flag of the parent must differ from the flag of the contour */
                assert(par_info->is_hole != is_hole);
                if(par_info->contour == 0)        /* removed contour */
                    goto resume_scan;
            }

            lnbd.x = x - is_hole;

            SaveMemStoragePos(scanner->storage2, &(scanner->backup_pos));

            seq = CreateSeq(scanner->seq_type1, scanner->header_size1,
                                   scanner->elem_size1, scanner->storage1);
            seq->flags |= is_hole ? CV_SEQ_FLAG_HOLE : 0;

            union {ContourInfo* ci; SetElem* se;} v;
            v.ci = l_cinfo;
            SetAdd(scanner->cinfo_set, 0, &v.se);
            l_cinfo = v.ci;
            int lval;

            if(img_i)
            {
                lval = img_i[x - is_hole] & 127;
                icvFetchContourEx_32s(img_i + x - is_hole, step_i,
                                      init_Point(origin.x + scanner->offset.x,
                                               origin.y + scanner->offset.y),
                                      seq, scanner->approx_method1,
                                      &(l_cinfo->rect));
            }
            else
            {
                lval = nbd;
                // change nbd
                nbd = (nbd + 1) & 127;
                nbd += nbd == 0 ? 3 : 0;
                icvFetchContourEx(img + x-is_hole, step,
                                   init_Point(origin.x + scanner->offset.x,
                                            origin.y + scanner->offset.y),
                                   seq, scanner->approx_method1,
                                   lval, &(l_cinfo->rect));
            }
            l_cinfo->rect.x -= scanner->offset.x;
            l_cinfo->rect.y -= scanner->offset.y;

            l_cinfo->next = scanner->cinfo_table[lval];
            scanner->cinfo_table[lval] = l_cinfo;
            l_cinfo->is_hole = is_hole;
            l_cinfo->contour = seq;
            l_cinfo->origin = origin;
            l_cinfo->parent = par_info;

            if(scanner->approx_method1 != scanner->approx_method2)
            {
                l_cinfo->contour = icvApproximateChainTC89((Chain *)seq,
                                                      scanner->header_size2,
                                                      scanner->storage2,
                                                      scanner->approx_method2);
                    ClearMemStorage(scanner->storage1);
            }

            l_cinfo->contour->v_prev = l_cinfo->parent->contour;

             if( par_info->contour == 0 )
            {
                l_cinfo->contour = 0;
                if(scanner->storage1 == scanner->storage2)
                {
                    RestoreMemStoragePos( scanner->storage1, &(scanner->backup_pos) );
                }
                else
                {
                    ClearMemStorage(scanner->storage1);
                }
                p = img[x];
                goto resume_scan;
            }

            SaveMemStoragePos( scanner->storage2, &(scanner->backup_pos2) );
            scanner->l_cinfo = l_cinfo;
            scanner->pt.x = !img_i ? x + 1 : x + 1 - is_hole;
            scanner->pt.y = y;
            scanner->lnbd = lnbd;
            scanner->img = (signed char*) img;
            scanner->nbd = nbd;
            return l_cinfo->contour;

        resume_scan:

            prev = p;
            /* update lnbd */
            if( prev & -2 )
            {
                lnbd.x = x;
            }
                            /* end of prev != p */
        }                   /* end of loop on x */

        lnbd.x = 0;
        lnbd.y = y + 1;
        x = 1;
        prev = 0;           /*end of loop on y */
    }                       

    return 0;
}

static void icvEndProcessContour(ContourScanner* scanner)
{
    ContourInfo* l_cinfo = scanner->l_cinfo;

    if(l_cinfo)
    {
        if(scanner->subst_flag)
        {
            MemStoragePos temp;

            SaveMemStoragePos(scanner->storage2, &temp);

            if(temp.top == scanner->backup_pos2.top &&
                temp.free_space == scanner->backup_pos2.free_space)
            {
                RestoreMemStoragePos(scanner->storage2, &scanner->backup_pos);
            }
            scanner->subst_flag = 0;
        }

        if(l_cinfo->contour)
        {
            cvInsertNodeIntoTree(l_cinfo->contour, l_cinfo->parent->contour,
                                  &(scanner->frame));
        }
        scanner->l_cinfo = 0;
    }
}

/*
   The function add to tree the last retrieved/substituted contour,
   releases temp_storage, restores state of dst_storage (if needed), and
   returns pointer to root of the contour tree */
Seq* EndFindContours(ContourScanner** _scanner)
{
    ContourScanner* scanner;
    Seq* first = 0;

    if(!_scanner)
        fatal("NULL Pointer Error");
    scanner = *_scanner;

    if(scanner)
    {
        icvEndProcessContour(scalar);

        if(scanner->storage1 != scanner->storage2)
            ReleaseMemStorage(&(scanner->storage1));

        if(scanner->cinfo_storage)
            ReleaseMemStorage(&(scanner->cinfo_storage));

        first = scanner->frame.v_next;
        cvFree(_scanner);
    }
    return first;
}

static ContourScanner* StartFindContours_Impl(Mat* mat, MemStorage* storage,
                     int  header_size, int mode,
                     int  method, Point offset, int needFillBorder)
{
    if(!storage)
        fatal("NULL Pointer Error");

    Point size = init_Point(mat->width, mat->height);
    int step = mat->step[0];
    unsigned char* img = (unsigned char*)(mat->data);
    
    if(header_size < sizeof(Contour))
        fatal("Bad size error");

    ContourScanner* scanner = cvAlloc(sizeof(*scanner));
    memset(scanner, 0, sizeof(*scanner));

    scanner->storage1 = scanner->storage2 = storage;
    scanner->img0 = (signed char*)img;
    scanner->img = (signed char*)(img + step);
    scanner->img_step = step;
    scanner->img_size.x = size.x - 1;   /* exclude rightest column */
    scanner->img_size.y = size.y - 1; /* exclude bottomost row */
    scanner->mode = mode;
    scanner->offset = offset;
    scanner->pt.x = scanner->pt.y = 1;
    scanner->lnbd.x = 0;
    scanner->lnbd.y = 1;
    scanner->nbd = 2;
    scanner->frame_info.contour = &(scanner->frame);
    scanner->frame_info.is_hole = 1;
    scanner->frame_info.next = 0;
    scanner->frame_info.parent = 0;
    scanner->frame_info.rect = init_Rect(0, 0, size.x, size.y);
    scanner->l_cinfo = 0;
    scanner->subst_flag = 0;

    scanner->frame.flags = 32768;

    scanner->approx_method2 = scanner->approx_method1 = method;

    if(method == 3 || method == 4)
        scanner->approx_method1 = 0;

    if(scanner->approx_method1 == 0)
    {
        scanner->seq_type1 = CV_SEQ_CHAIN_CONTOUR;
        scanner->header_size1 = scanner->approx_method1 == scanner->approx_method2 ?
            header_size : sizeof(Chain);
        scanner->elem_size1 = sizeof(char);
    }
    else
    {
        scanner->seq_type1 = CV_SEQ_POLYGON;
        scanner->header_size1 = scanner->approx_method1 == scanner->approx_method2 ?
            header_size : sizeof(Contour);
        scanner->elem_size1 = sizeof(Point);
    }

    scanner->header_size2 = header_size;

    if( scanner->approx_method2 == 0 )
    {
        scanner->seq_type2 = scanner->seq_type1;
        scanner->elem_size2 = scanner->elem_size1;
    }
    else
    {
        scanner->seq_type2 = CV_SEQ_POLYGON;
        scanner->elem_size2 = sizeof(Point);
    }

    scanner->seq_type1 = scanner->approx_method1 == 0 ?
        CV_SEQ_CHAIN_CONTOUR : CV_SEQ_POLYGON;

    scanner->seq_type2 = scanner->approx_method2 == 0 ?
        CV_SEQ_CHAIN_CONTOUR : CV_SEQ_POLYGON;

    SaveMemStoragePos(storage, &(scanner->initial_pos));

    if(method > 2)
        scanner->storage1 = CreateChildMemStorage(scanner->storage2);

    if(mode > 1) 
    {
        scanner->cinfo_storage = CreateChildMemStorage(scanner->storage2);
        scanner->cinfo_set = CreateSet(0, sizeof(Set), sizeof(ContourInfo),
                                          scanner->cinfo_storage);
    }

    assert(step >= 0);
    assert(size.y >= 1);

    /* make zero borders */
    if(needFillBorder)
    {
        int esz = CV_ELEM_SIZE(mat->type);
        memset(img, 0, size.width*esz);
        memset(img + (size_t)(step)*(size.y-1), 0, size.x*esz);

        img += step;
        for(int y = 1; y < size.y-1; y++, img += step)
        {
            for(int k = 0; k < esz; k++)
                img[k] = img[(size.width-1)*esz+k] = (signed char)0;
        }
    }

    if(CV_MAT_TYPE(mat->type) != 4)
        cvThreshold(mat, mat, 0, 1, 0);

    return scanner;
}

static int FindContours_Impl(Mat*  img,  MemStorage*  storage,
                Seq**  firstContour, int  cntHeaderSize,
                int  mode,
                int  method, CvPoint offset, int needFillBorder)
{
    ContourScanner* scanner = 0;
    Seq *contour = 0;
    int count = -1;

    if(!firstContour)
        fatal("NULL double Seq pointer");

    *firstContour = 0;
    scanner = StartFindContours_Impl(img, storage, cntHeaderSize, mode, method, offset,
                                            needFillBorder);

    do
    {
        count++;
        contour = FindNextContour(scanner);
    }
    while(contour != 0);

    *firstContour = EndFindContours(&scanner);
    return count;
}

void findContours(Mat image0, vector* contours, vector* hierarchy, int mode, int method, Point offset)
{
    Point offset0 = init_Point(-1, -1);
    Scalar s = init_Scalar(0, 0, 0, 0);
    Mat* image = malloc(sizeof(Mat));
    copyMakeBorder(image0, image, 1, 1, 1, 1, 16, s);
    MemStorage* storage = malloc(sizeof(MemStorage));
    init_MemStorage(storage, 0);
    Seq* _ccontours = 0;
    vector_free(hierarchy);
    FindContours_Impl(image, storage, &_ccontours, sizeof(Contour), mode, method, init_Point(offset.x + offset0.x, offset.y + offset0.y), 0);

    if(!_ccontours)
    {
        vector_free(*contours);
        return;
    }
    Seq* all_contours = TreeToNodeSeq(_ccontours, sizeof(Seq), storage);
    int i, total = all_contours ? all_contours->total, 0;
    createVectorOfVector(contours, total, 1, 0, -1);
    SeqIterator it = begin_it(all_contours);
    for( i = 0; i < total; i++, getNextIterator_Seq(&it))
    {
        Seq* c = (Seq*)it.ptr;
        ((Contour*)c)->color = i;
        createVectorOfVector(contours, (int)c->total, 1, CV_MAKETYPE(4, 2), i);
        Mat ci = getMatfromVector(vector_get(contours, i));
        CvtSeqToArray(c, ptr(ci, 0), init_Point(0, 0x3fffffff));
    }

    vector_resize(hierarchy, total);
	it = begin_it(all_contours);
    for(i = 0; i < total; i++, getNextIterator_Seq(&it))
    {
        Seq* c = (Seq*)it.ptr;
        int h_next = c->h_next ? ((Contour*)c->h_next)->color : -1;
        int h_prev = c->h_prev ? ((Contour*)c->h_prev)->color : -1;
        int v_next = c->v_next ? ((Contour*)c->v_next)->color : -1;
        int v_prev = c->v_prev ? ((Contour*)c->v_prev)->color : -1;
        *(Scalar*)vector_get(hierarchy, i) = init_Scalar(h_next, h_prev, v_next, v_prev);
    }    
}
