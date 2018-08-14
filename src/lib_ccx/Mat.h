#ifndef MAT_H
#define MAT_H

#include "math.h"
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define CV_MAT_CN(flags)        ((((flags) & 4088) >> 3) + 1)
#define CV_MAT_DEPTH(flags)     ((flags) & 7)
#define CV_MAT_TYPE(flags)      ((flags) & 4095)
#define CV_MAKETYPE(depth,cn) (CV_MAT_DEPTH(depth) + (((cn)-1) << 3))

/** Size of each channel item,
   0x8442211 = 1000 0100 0100 0010 0010 0001 0001 ~ array of sizeof(arr_type_elem) */
#define CV_ELEM_SIZE1(type) \
    ((((sizeof(size_t)<<28)|0x8442211) >> (CV_MAT_DEPTH(type))*4) & 15)

/** 0x3a50 = 11 10 10 01 01 00 00 ~ array of log2(sizeof(arr_type_elem)) */
#define CV_ELEM_SIZE(type) \
    (CV_MAT_CN(type) << ((((sizeof(size_t)/4+1)*16384|0x3a50) >> CV_MAT_DEPTH(type)*2) & 3))

typedef struct Mat
{
	/*! includes several bit-fields:
	     - the magic signature
	     - continuity flag
	     - depth
	     - number of channels
	*/
	int flags;
	//! the number of rows and columns
	int rows, cols;
	//! pointer to the data
	unsigned char* data;

    //! helper fields used in locateROI and adjustROI
    const unsigned char* datastart;
    const unsigned char* dataend;
    const unsigned char* datalimit;

    size_t step[2];
} Mat ;

typedef struct  MatIterator
{
    //! the iterated arrays
    const Mat** arrays;
    //! data pointers
    unsigned char** ptrs;
    //! the number of arrays
    int narrays;
    //! the number of hyper-planes that the iterator steps through
    size_t nplanes;
    //! the size of each segment (in elements)
    size_t size;

    int iterdepth;
    size_t idx;
} MatIterator ;

typedef struct MatConstIterator
{
    const Mat* m;
    size_t elemSize;
    const unsigned char* ptr;
    const unsigned char* sliceStart;
    const unsigned char* sliceEnd;
} MatConstIterator ;

typedef union Cv32suf
{
    int i;
    unsigned u;
    float f;
    struct _fp32Format
    {
        unsigned int significand : 23;
        unsigned int exponent    : 8;
        unsigned int sign        : 1;
    } fmt;
} Cv32suf ;

Mat emptyMat();
void create(Mat* m, int _rows, int _cols, int _type);
Mat createMat(int _rows, int _cols, int _type, void* _data, size_t _step/* 0 */);
Mat createusingRange(Mat m, int row0, row1);
Mat createusingRect(Mat _m, Rect roi);
void createusingScalar(Mat* m, Scalar s);
void createVectorMat(vector* v/* Mat */, int rows, int cols, int mtype, int i/* -1 */);
void leftshift_op(Mat* m, int cols, float sample[]);
int updateContinuityFlag(Mat* m);
size_t elemSize(Mat m);
size_t elemSize1(Mat m);
bool isContinuous(Mat m);
bool isSubmatrix(Mat m);
void release(Mat* m);
int type(Mat m);
bool empty(Mat m);
int depth(Mat m);
int channels(Mat m);
int total(Mat m);
unsigned char* ptr(Mat m, int i);
Point getContinuousSize(Mat m1, Mat m2, int widthScale);
void copyTo(Mat src, Mat* dst);
void convertTo(Mat src, Mat* dst, int _type, double alpha/*1*/, double beta/*0*/, int code);
void setTo(Mat* m, Scalar s);
Mat row_op(Mat m, Point rowRange);
void divide_op(Mat* m, double s);
void minus_op(Mat* m, double s, double alpha);
void zeros(Mat* m, int _rows, int _cols, int _type);
void getNextIterator(MatIterator* it);
void split(Mat m, vector* dst/* Mat */);
void get_gradient_magnitude(Mat* _grey_image, Mat* _gradient_magnitude);
void convexHull(Mat points, vector* hull, bool clockwise /* false */, bool returnPoints/* true */);
MatIterator matIterator(const Mat** _arrays, unsigned char** _ptrs, int _narrays);
void subtract(const Mat src1, const Scalar psrc2, Mat* dst);
int checkVector(Mat m, int _elemChannels, int _depth /* -1 */, bool _requireContinuous/* true */);
Rect boundingRect(vector* v/* Point */)
void meanStdDev(Mat* src, Scalar* mean, Scalar* sdv, Mat* mask);
int countNonZero(Mat src);
void copyMakeBorder(Mat src, Mat* dst, int top, int bottom, int left, int right, int borderType, Scalar value);
void getNextIterator_Seq(SeqIterator* it);
Mat getMatfromVector(vector* v);
void createVectorOfVector(vector* v, int rows, int cols, int mtype, int i);

void vector_init(vector *v)
{
   if(!v)
      v = malloc(sizeof(vector));
   v->capacity = 1;
   v->items = malloc(sizeof(void*));
   v->size = 0;
}

void vector_init_n(vector *v, int capacity)
{
   if(!v)
      v = malloc(sizeof(vector));
   v->size = 0;
   v->capacity = capacity;
   v->items = malloc(sizeof(void*) * capacity);
}

int vector_size(vector *v)
{
   if(!v)
      vector_init(v);
   return v->size;
}

static void vector_resize(vector *v, int capacity)
{
   if(!v)
      vector_init(v);
   void **items;
   if(capacity == 0)
   {
      capacity = 1;
      items = malloc(sizeof(void *) * 1);
   }
   else
      items = realloc(v->items,sizeof(void *) * capacity);
   if(items)
   {
      v->items = items;
      v->capacity = capacity;
   }
}

void vector_add(vector *v, void *item)
{
   if(!v)
      vector_init_n(v, 1);

   if(v->capacity == 0)
   {
      v->items = malloc(sizeof(void*));
      v->capacity = 1;
   }

   if(v->size == v->capacity)
      vector_resize(v, v->capacity * 2);

   v->items[v->size++] = item;
}

void vector_addfront(vector *v, void *item)
{
   if(!v)
      vector_init(v);

   if(v->size == v->capacity)
      vector_resize(v, v->capacity * 2);

   v->size++;
   for(int i = 1; i < v->size; i++)
      v->items[i] = v->items[i-1];
   v->items[0] = item;
}

void vector_set(vector *v, int index, void *item) 
{
   if(!v)
      vector_init_1(v);
   if(index >= 0 && index < v->size)
      v->items[index] = item;
}

void *vector_get(vector *v, int index)
{
   if(index >= 0 && index < v->size)
      return v->items[index];
   return NULL;
}

void vector_delete(vector *v, int index)
{

   if(index < 0 || index >= v->size)
      return;

   for(int i = index; i < v->size-1; i++)
      v->items[i] = v->items[i+1];

   v->items[v->size-1] = NULL;
   v->size--;
   if(v->size > 0 && v->size == v->capacity/4)
      vector_resize(v, v->capacity/2);
}

void vector_free(vector* v)
{
   free(v->items);
   v->size = 0;
   v->capacity = 0;
   free(v);
}

void *vector_front(vector *v)
{
   if(!v->size)
      return NULL;
   return v->items[0];
}

void *vector_back(vector *v)
{
   if(!v->size)
      return NULL;
   return v->items[v->size-1];
}

bool vector_empty(vector *v)
{
   if(!v)
      return true;
   return (v->size == 0);
}

void vector_swap(vector* v, vector* u)
{
    vector temp;
    temp = *v;
    *v = *u;
    *u = temp;
}

void vector_assign(vector* v, vector* u)
{
   if(!v)
      vector_init(v);

   if(!u)
      vector_init(u);

   *u = *v;
}

void vector_clear(vector* v)
{
   v->size = 0;
   free(v->items);
}

#endif
