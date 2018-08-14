#ifndef MATH_H
#define MATH_H

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define CV_PI   3.1415926535897932384626433832795
#define CV_2PI  6.283185307179586476925286766559
#define CV_LOG2 0.69314718055994530941723212145818
#define CV_SIGN(a) (((a) > (0)) - ((a) < (0)))

typedef struct vector
{
   void **items;
   int capacity;
   int size;
} vector;

void vector_init(vector *v);
void vector_init_n(vector* v, int capacity);
int vector_size(vector *v);
static void vector_resize(vector *v, int capacity);
void vector_add(vector *v, void *item);
void vector_set(vector *v, int idx, void *item);
void *vector_get(vector *v, int idx);
void vector_delete(vector *v, int idx);
void vector_free(vector *v);
void *vector_front(vector *v);
void *vector_back(vector *v);
bool vector_empty(vector *v);

typedef struct Scalar
{
    double val[4];
} Scalar ;

Scalar init_Scalar(double v1, double v2, double v3, double v4)
{
    Scalar s;
    s.val[0] = v1;
    s.val[1] = v2;
    s.val[2] = v3;
    s.val[3] = v4;
    return s;
}

typedef struct AutoBuffer
{
    size_t fixed_size;
    //! pointer to the real buffer, can point to buf if the buffer is small enough
    void* ptr;
    //! size of the real buffer
    size_t sz;
    //! pre-allocated buffer. At least 1 element to confirm C++ standard requirements
    void* buf;
} AutoBuffer ;

typedef struct Point
{
    int x;
    int y;
} Point ;

Point init_Point(int _x, int _y)
{
    Point p;
    p.x = _x;
    p.y = _y;
    return p;
}

typedef struct Size
{
    int width;
    int height;
} Size ;

Size init_size(int _width, int _height)
{
    Size sz;
    sz.width = _width;
    sz.height = _height;
    return sz;
}

typedef struct Vec3i
{
    int val[3];
} Vec3i ;

Vec3i vec3i(int a, int b, int c)
{
    Vec3i v;
    v.val[0] = a;
    v.val[1] = b;
    v.val[2] = c;
    return v;
}

typedef struct Rect
{
	int x;					  //!< x coordinate of the top-left corner
	int y;					  //!< y coordinate of the top-left corner 
	int width;				  //!< width of the rectangle
	int height;				  //!< height of the rectangle
} Rect ;

Rect init_Rect(int _x, int _y, int _width, int _height)
{
    Rect r;
	r->x = _x;
	r->y = _y;
	r->width = _width;
	r->height = _height;
    return r;
}

Rect createRect(Point p, Point q)
{
    Rect r;
    r.x = min(p.x, q.x);
    r.y = min(p.y, q.y);
    r.width = max(p.x, q.x) - r.x;
    r.height = max(p.y, q.y) - r.y;
    return r;
}

//top-left
Point tl(Rect rect)
{
    return init_Point(rect.x, rect.y);
}

//bottom-right
Point br(Rect rect)
{
    return init_Point(rect.x + rect.width, rect.y + rect.height);
}

Rect performOR(Rect* a, Rect* b)
{
    if(a->width <= 0 || a->height <= 0)
        a = b;

    else if(b->width > 0 && b->height > 0)
    {
        int x1 = min(a->x, b->x);
        int y1 = min(a->y, b->y);
        a->width = max(a->x + a->width, b->x + b->width) - x1;
        a->height = max(a->y + a->height, b->y + b->height) - y1;
        a->x = x1;
        a->y = y1;
    }
    return *a;
}

bool equalRects(Rect a, Rect b)
{
    return (a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height);
}

// struct line_estimates
// Represents a line estimate (as above) for an ER's group
// i.e.: slope and intercept of 2 top and 2 bottom lines
typedef struct line_estimates
{
    float top1_a0;
    float top1_a1;
    float top2_a0;
    float top2_a1;
    float bottom1_a0;
    float bottom1_a1;
    float bottom2_a0;
    float bottom2_a1;
    int x_min;
    int x_max;
    int h_max;
} line_estimates ;

// Represents a pair of ER's
typedef struct region_pair
{
    Point a;
    Point b;
} region_pair ;

region_pair init_region_pair(Point a, Point b)
{
    region_pair pair;
    pair.a = a;
    pair.b = b;
    return pair;
}

bool equalRegionPairs(region_pair r1, region_pair r2)
{
    return r1.a.x == r2.a.x && r1.a.y == r2.a.y && r1.b.x == r2.b.x && r1.b.y == r2.b.y;
}

// struct region_triplet
// Represents a triplet of ER's
typedef struct region_triplet
{
    Point a;
    Point b;
    Point c;
    line_estimates estimates;
} region_triplet ;

region_triplet init_region_triplet(Point _a, Point _b, Point _c)
{
    region_triplet triplet;
    triplet.a = _a;
    triplet.b = _b;
    triplet.c = _c;
    return triplet;
}

// struct region_sequence
// Represents a sequence of more than three ER's
typedef struct region_sequence
{
    vector* triplets; //region_triplet
} region_sequence ;

region_sequence Region_sequence(region_triplet* t)
{
    region_sequence sequence;
    sequence.triplets = malloc(sizeof(vector));
    vector_add(sequence.triplets, t);
    return sequence;
}

bool haveCommonRegion_triplet(region_triplet t1, region_triplet t2)
{
    if( (t1.a.x ==t2.a.x && t1.a.y == t2.a.y) || (t1.a.x ==t2.b.x && t1.a.y == t2.b.y) || (t1.a.x ==t2.c.x && t1.a.y == t2.c.y) ||
        (t1.b.x ==t2.a.x && t1.b.y == t2.a.y) || (t1.b.x ==t2.b.x && t1.b.y == t2.b.y) || (t1.b.x ==t2.c.x && t1.b.y == t2.c.y) ||
        (t1.c.x ==t2.a.x && t1.c.y == t2.a.y) || (t1.c.x ==t2.b.x && t1.c.y == t2.b.y) || (t1.c.x ==t2.b.x && t1.c.y == t2.b.y) )
        return true;

    retur false;
}

// Check if two sequences share a region in common
bool haveCommonRegion(region_sequence sequence1, region_sequence sequence2)
{
    for (size_t i=0; i < vector_size(sequence2.triplets); i++)
    {
        for (size_t j=0; j < vector_size(sequence1.triplets); j++)
        {
            if (haveCommonRegion_triplet(*(region_triplet*)vector_get(sequence2.triplets, i), *(region_triplet*)vector_get(sequence1.triplets, j)));
                return true;
        }
    }

    return false;
}

int min(int x1, int x2)
{
    return x1 > x2 ? x2 : x1;
}

int max(double x1, double x2)
{
    return x1 < x2 ? x2 : x1;
}

void swap(int *v1, int* v2)
{
    int temp = *v1;
    *v1 = *v2;
    *v2 = temp;
}

void swap_size(Size* sz1, Size* sz2)
{
    Size temp = *sz1;
    *sz1 = *sz2;
    *sz2 = temp;
}

void swap_uchar(unsigned char** ptr1, unsigned char** ptr2)
{
    unsigned char* temp = *ptr1;
    *ptr1 = *ptr2;
    *ptr2 = temp;
}

void sort_3ints(vector *v)
{
    if (*(int*)vector_get(v, 0) > *(int*)vector_get(v, 1))
        swap(vector_get(v, 0), vector_get(v, 1));

    if (*(int*)vector_get(v, 1) > *(int*)vector_get(v, 2))
        swap(vector_get(v, 1), vector_get(v, 2));

    if (*(int*)vector_get(v, 0) > *(int*)vector_get(v, 1))
        swap(vector_get(v, 0), vector_get(v, 1));
}

int norm(int x, int y)
{
    return sqrt(x*x + y*y);
}

bool vector_contains(vector* v, Point a) //Checks specifically for the struct Point
{
    for(int i = 0; i < vector_size(v); i++)
    {
        Point pt = *(Point*)vector_get(v, i);
        if(pt.x == a.x && pt.y == a.y)
            return true;
    }
    return false;
}

int sort_couples(const void* i, const void* j) 
{
    return (*(Vec3i*)i).val[0] - (*(Vec3i*)j).val[0]; 
}

void sort(vector* v /* Vec3i */)
{
    int len = vector_size(v);
    Vec3i arr[len];

    for(int i = 0; i < len; i++)
        arr[i] = *(Vec3i*)vector_get(v, i);

    vector_init(v);
    qsort(arr, len, sizeof(Vec3i), sort_couples);

    for(int i = 0; i < len; i++)
        vector_add(v, &arr[i]);
}

int floor(double value)
{
    int i = (int)value;
    return i - (i > value);
}

int round(double value)
{
    /* it's ok if round does not comply with IEEE754 standard;
       the tests should allow +/-1 difference when the tested functions use round */
    return (int)(value + (value >= 0 ? 0.5 : -0.5));
}

// Fit line from two points
// out a0 is the intercept
// out a1 is the slope
void fitLine(Point p1, Point p2, float* a0, float* a1)
{
    *a1 = (float)(p2.y - p1.y) / (p2.x - p1.x);
    *a0 = *a1 * -1 * p1.x + p1.y;
}


// Fit a line_estimate to a group of 3 regions
// out triplet->estimates is updated with the new line estimates
bool fitLineEstimates(vector* regions/* vector<ERStat> */, region_triplet* triplet)
{
    vector* char_boxes = malloc(sizeof(vector));
    vector_init(char_boxes);
    vector_add(char_boxes, &((*(ERStat*))vector_get(vector_get(regions, triplet->a.x), triplet->a.y)).rect);
    vector_add(char_boxes, &((*(ERStat*))vector_get(vector_get(regions, triplet->b.x), triplet->b.y)).rect);
    vector_add(char_boxes, &((*(ERStat*))vector_get(vector_get(regions, triplet->c.x), triplet->c.y)).rect);
    
    triplet->estimates.x_min = min(min((Rect*)vector_get(char_boxes, 0)->x,(Rect*)vector_get(char_boxes, 1)->x), (Rect*)vector_get(char_boxes, 2)->x);
    triplet->estimates.x_max = max(max((Rect*)vector_get(char_boxes, 0)->x + (Rect*)vector_get(char_boxes, 0)->width, (Rect*)vector_get(char_boxes, 1)->x + (Rect*)vector_get(char_boxes, 1)->width), (Rect*)vector_get(char_boxes, 2)->x + (Rect*)vector_get(char_boxes, 2)->width);
    triplet->estimates.h_max = max(max((Rect*)vector_get(char_boxes, 0)->height, (Rect*)vector_get(char_boxes, 0)->height), (Rect*)vector_get(char_boxes, 0)->x);

    // Fit one bottom line
    float err = fitLineLMS(init_Point((Rect*)vector_get(char_boxes, 0)->x + (Rect*)vector_get(char_boxes, 0)->width, (Rect*)vector_get(char_boxes, 0)->y + (Rect*)vector_get(char_boxes, 0)->height),
                           init_Point((Rect*)vector_get(char_boxes, 1)->x + (Rect*)vector_get(char_boxes, 1)->width, (Rect*)vector_get(char_boxes, 1)->y + (Rect*)vector_get(char_boxes, 1)->height),
                           init_Point((Rect*)vector_get(char_boxes, 2)->x + (Rect*)vector_get(char_boxes, 2)->width, (Rect*)vector_get(char_boxes, 2)->y + (Rect*)vector_get(char_boxes, 2)->height), 
                           &(triplet->estimates.bottom1_a0), &(triplet->estimates.bottom1_a1));

    if ((triplet->estimates.bottom1_a0 == -1) && (triplet->estimates.bottom1_a1 == 0))
        return false;

    // Slope for all lines must be the same
    triplet->estimates.bottom2_a1 = triplet->estimates.bottom1_a1;
    triplet->estimates.top1_a1    = triplet->estimates.bottom1_a1;
    triplet->estimates.top2_a1    = triplet->estimates.bottom1_a1;

    if (abs(err) > (float)triplet->estimates.h_max/6)
    {
        // We need two different bottom lines
        triplet->estimates.bottom2_a0 = triplet->estimates.bottom1_a0 + err;
    }
    else
    {
        // Second bottom line is the same
        triplet->estimates.bottom2_a0 = triplet->estimates.bottom1_a0;
    }

    // Fit one top line within the two (Y)-closer coordinates
    int d_12 = abs((Rect*)vector_get(char_boxes, 0)->y - (Rect*)vector_get(char_boxes, 1)->y);
    int d_13 = abs((Rect*)vector_get(char_boxes, 0)->y - (Rect*)vector_get(char_boxes, 2)->y);
    int d_23 = abs((Rect*)vector_get(char_boxes, 1)->y - (Rect*)vector_get(char_boxes, 2)->y);
    if((d_12 < d_13) && (d_12 < d_23))
    {
        Point p = init_Point(((Rect*)vector_get(char_boxes, 0)->x + (Rect*)vector_get(char_boxes, 1)->x)/2,
                        ((Rect*)vector_get(char_boxes, 0)->y + (Rect*)vector_get(char_boxes, 1)->y)/2);

        triplet->estimates.top1_a0 = triplet->estimates.bottom1_a0 +
                (p.y - (triplet->estimates.bottom1_a0+p.x*triplet->estimates.bottom1_a1));

        p = init_Point((Rect*)vector_get(char_boxes, 2)->x, (Rect*)vector_get(char_boxes, 2)->y);
        err = (p.y - (triplet->estimates.top1_a0+p.x*triplet->estimates.top1_a1));
    }
    else if(d_13 < d_23)
    {
        Point p = init_Point(((Rect*)vector_get(char_boxes, 0)->x + (Rect*)vector_get(char_boxes, 2)->x)/2,
                        ((Rect*)vector_get(char_boxes, 0)->y + (Rect*)vector_get(char_boxes, 2)->y)/2);
        triplet->estimates.top1_a0 = triplet->estimates.bottom1_a0 +
                (p.y - (triplet->estimates.bottom1_a0+p.x*triplet->estimates.bottom1_a1));

        p = init_Point((Rect*)vector_get(char_boxes, 1)->x, (Rect*)vector_get(char_boxes, 1)->y);
        err = (p.y - (triplet->estimates.top1_a0+p.x*triplet->estimates.top1_a1));
    }
    else
    {
        Point p = init_Point(((Rect*)vector_get(char_boxes, 1)->x + (Rect*)vector_get(char_boxes, 2)->x)/2,
                        ((Rect*)vector_get(char_boxes, 1)->y + (Rect*)vector_get(char_boxes, 2)->y)/2);

        triplet->estimates.top1_a0 = triplet->estimates.bottom1_a0 +
                (p.y - (triplet->estimates.bottom1_a0+p.x*triplet->estimates.bottom1_a1));

        p = init_Point((Rect*)vector_get(char_boxes, 0)->x, (Rect*)vector_get(char_boxes, 0)->y);
        err = (p.y - (triplet->estimates.top1_a0+p.x*triplet->estimates.top1_a1));
    }

    if (abs(err) > (float)triplet->estimates.h_max/6)
    {
        // We need two different top lines
        triplet->estimates.top2_a0 = triplet->estimates.top1_a0 + err;
    }
    else
    {
        // Second top line is the same
        triplet->estimates.top2_a0 = triplet->estimates.top1_a0;
    }

    return true;
}

// Fit line from three points using (heuristic) Least-Median of Squares
// out a0 is the intercept
// out a1 is the slope
// returns the error of the single point that doesn't fit the line
float fitLineLMS(Point p1, Point p2, Point p3, float* a0, float* a1)
{
    //if this is not changed the line is not valid
    *a0 = -1;
    *a1 = 0;

    //Least-Median of Squares does not make sense with only three points
    //because any line passing by two of them has median_error = 0
    //So we'll take the one with smaller slope
    float l_a0, l_a1, best_slope=FLT_MAX, err=0;

    if(p1.x != p2.x)
    {
        fitLine(p1,p2,l_a0,l_a1);
        if(abs(l_a1) < best_slope)
        {
            best_slope = abs(l_a1);
            a0 = l_a0;
            a1 = l_a1;
            err = (p3.y - (a0+a1*p3.x));
        }
    }

    if(p1.x != p3.x)
    {
        fitLine(p1,p3,l_a0,l_a1);
        if(abs(l_a1) < best_slope)
        {
            best_slope = abs(l_a1);
            a0 = l_a0;
            a1 = l_a1;
            err = (p2.y - (a0+a1*p2.x));
        }
    }

    if(p2.x != p3.x)
    {
        fitLine(p2,p3,l_a0,l_a1);
        if(abs(l_a1) < best_slope)
        {
            best_slope = abs(l_a1);
            a0 = l_a0;
            a1 = l_a1;
            err = (p1.y - (a0+a1*p1.x));
        }
    }

    return err;
}

AutoBuffer init_AutoBuffer(size_t _size)
{
    AutoBuffer ab;
    ab.fixed_size = 1024/sizeof(int)+8;
    ab.buf = (int*)malloc((ab.fixed_size > 0) ? ab.fixed_size : 1);
    ab.ptr = ab.buf;
    ab.sz = ab.fixed_size;
    allocateAB(&ab, _size);
    return ab;
}

AutoBuffer init_AutoBufferPointer(size_t _size)
{
    AutoBuffer ab;
    ab.fixed_size = 1024/sizeof(Point*)+8;
    ab.buf = (Point**)malloc((ab.fixed_size > 0) ? ab.fixed_size : 1);
    ab.ptr = ab.buf;
    ab.sz = ab.fixed_size;
    allocateAB(&ab, _size);
    return ab;
}

AutoBuffer init_AutoBufferPoint(size_t _size)
{
    AutoBuffer ab;
    ab.fixed_size = 1024/sizeof(int)+8;
    ab.buf = (Point*)malloc((ab.fixed_size > 0) ? ab.fixed_size : 1);
    ab.ptr = ab.buf;
    ab.sz = ab.fixed_size;
    allocateAB(&ab, _size);
    return ab;
}

AutoBuffer init_AutoBufferPoint_(size_t size)
{
    AutoBuffer ab;
    ab.fixed_size = 1024/sizeof(Point*)+8;
    ab.buf = (Point**)malloc((ab.fixed_size > 0) ? ab.fixed_size : 1);
    ab.ptr = ab.buf;
    ab.sz = ab.fixed_size;
    allocateAB(&ab, _size);
    return ab;
}

AutoBuffer init_AutoBufferdouble(size_t _size)
{
    AutoBuffer ab;
    ab.fixed_size = 1024/sizeof(double)+8;
    ab.buf = (double*)malloc((ab.fixed_size > 0) ? ab.fixed_size : 1);
    ab.ptr = ab.buf;
    ab.sz = ab.fixed_size;
    allocateAB(&ab, _size);
    return ab;
}

AutoBuffer init_AutoBufferuchar(size_t _size)
{
    AutoBuffer ab;
    ab.fixed_size = 1024/sizeof(unsigned char)+8;
    ab.buf = (unsigned char*)malloc((ab.fixed_size > 0) ? ab.fixed_size : 1);
    ab.ptr = ab.buf;
    ab.sz = ab.fixed_size;
    allocateAB(&ab, _size);
    return ab;
}

AutoBuffer init_AutoBufferPt(size_t _size)
{
    AutoBuffer ab;
    ab.fixed_size = 1024/sizeof(int)+8;
    ab.buf = (PtInfo*)malloc((ab.fixed_size > 0) ? ab.fixed_size : 1);
    ab.ptr = ab.buf;
    ab.sz = ab.fixed_size;
    allocateAB(&ab, _size);
    return ab;
}

void allocateAB(AutoBuffer* ab, size_t _size)
{
    if(_size <= ab->sz)
    {
        ab->sz = _size;
        return;
    }
    deallocateAB(ab);
    ab->sz = _size;
    if(_size > ab->fixed_size)
        ab->ptr = malloc(_size);
}

void deallocateAB(AutoBuffer* ab)
{
    if(ab->ptr != ab->buf)
    {
        free(ab->ptr);
        ab->ptr = ab->buf;
        ab->sz = ab->fixed_size;
    }
}

inline void AutoBuffer_resize(AutoBuffer* ab, size_t _size)
{
    if(_size <= ab->sz)
    {
        ab->sz = _size;
        return;
    }
    size_t i, prevsize = sz, minsize = min(prevsize, _size);
    void* prevptr = ab->ptr;

    ab->ptr = _size > ab->fixed_size ? malloc(_size) : buf;
    ab->sz = _size;

    if(ptr != prevptr)
        for(i = 0; i < minsize; i++ )
            ab->ptr[i] = prevptr[i];
    for(i = prevsize; i < _size; i++)
        ab->ptr[i] = init_Point(0, 0);

    if(prevptr != ab->buf)
        free(prevptr);
}

void scalarToRawData(const Scalar s, unsigned char* buf, int type, int unroll_to)
{
    int i;
    const int depth = CV_MAT_DEPTH(type), cn = CV_MAT_CN(type);
    for(i = 0; i < cn; i++)
        buf[i] = s.val[i];

    for(; i < unroll_to; i++)
        buf[i] = buf[i-cn];
}

void scalartoRawData(const Scalar s, double* buf, int type, int unroll_to)
{
    int i;
    const int depth = CV_MAT_DEPTH(type), cn = CV_MAT_CN(type);
    for(i = 0; i < cn; i++)
        buf[i] = s.val[i];

    for(; i < unroll_to; i++)
        buf[i] = buf[i-cn];
}

#endif