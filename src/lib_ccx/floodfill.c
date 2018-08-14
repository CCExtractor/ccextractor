#include "types.h"
#include "Mat.h"
#include "Math.h"

static void floodFill_CnIR(Mat* image, Point seed, unsigned char newVal, ConnectedComp* region, int flags, vector* buffer)
{
    unsigned char* img = image->data + (image->step)[0] * seed.y;
    int roi[] = {image.rows, image.cols};
    int i, L, R;
    int area = 0;
    int XMin, XMax, YMin = seed.y, YMax = seed.y;
    int _8_connectivity = (flags & 255) == 8;
    FFillSegment* buffer_end = vector_front(buffer) + vector_size(buffer), *head = vector_front(buffer), *tail = vector_front(buffer);

    L = R = XMin = XMax = seed.x;

    unsigned char val0 = img[L];
    img[L] = newVal;

    while(++R < roi[1] && img[R] == val0)
        img[R] = newVal;

    while( --L >= 0 && img[L] == val0 )
        img[L] = newVal;

    XMax = --R;
    XMin = ++L;

    tail->y = (unsigned short)(seed.y);
    tail->l = (unsigned short)(L);
    tail->r = (unsigned short)(R);
    tail->prevl = (unsigned short)(R+1);
    tail->prevr = (unsigned short)(R);
    tail->dir = (short)(UP);
    if(++tail == buffer_end)
    {                                             
        vector_resize(buffer, vector_size(buffer) * 3/2);
        tail = vector_front(buffer) + (tail - head);
        head = vector_front(buffer);
        buffer_end = head + vector_size(buffer);
    }

    while(head != tail)
    {
        int k, YC, PL, PR, dir;

        --tail;
        YC = tail->y;                                  
        L = tail->l;                                  
        R = tail->r;                                 
        PL = tail->prevl;                         
        PR = tail->prevr;                         
        dir = tail->dir;

        int data[][3] =
        {
            {-dir, L - _8_connectivity, R + _8_connectivity},
            {dir, L - _8_connectivity, PL - 1},
            {dir, PR + 1, R + _8_connectivity}
        };

        if(region)
        {
            area += R - L + 1;

            if(XMax < R) XMax = R;
            if(XMin > L) XMin = L;
            if(YMax < YC) YMax = YC;
            if(YMin > YC) YMin = YC;
        }

        for( k = 0; k < 3; k++ ) 
        {
            dir = data[k][0];

            if((unsigned)(YC + dir) >= (unsigned)roi[0])
                continue;

            img = ptr(*img, YC + dir);
            int left = data[k][1];
            int right = data[k][2];

            for(i = left; i <= right; i++)
            {
                if((unsigned)i < (unsigned)roi[1] && img[i] == val0)
                {
                    int j = i;
                    img[i] = newVal;
                    while(--j >= 0 && img[j] == val0)
                        img[j] = newVal;

                    while(++i < roi[1] && img[i] == val0)
                        img[i] = newVal;

                    tail->y = (unsigned short)(YC + dir);                        
                    tail->l = (unsigned short)(j+1);
                    tail->r = (unsigned short)(i-1);                        
                    tail->prevl = (unsigned short)(L);               
                    tail->prevr = (unsigned short)(R);               
                    tail->dir = (short)(-dir);                     
                    if(++tail == buffer_end)                    
                    {                                             
                        vector_resize(buffer, vector_size(buffer) * 3/2);
                        tail = vector_front(buffer) + (tail - head);  
                        head = vector_front(buffer);
                        buffer_end = head + vector_size(buffer);
                    }
                }
            }
        }
    }

    if(region)
    {
        region->pt = seed;
        region->area = area;
        region->rect.x = XMin;
        region->rect.y = YMin;
        region->rect.width = XMax - XMin + 1;
        region->rect.height = YMax - YMin + 1;
    }
}

void floodFillGrad_CnIR(Mat* image, Mat* msk,
                   Point seed, unsigned char newVal, unsigned char newMaskVal,
                   Diff8uC1 diff, ConnectedComp* region, int flags,
                   vector* buffer) //<unsigned char, unsigned char, int, Diff8uC1>
{
    int step = (image->step)[0], maskStep = (msk->step)[0];
    unsigned char* pImage = ptr(*image, 0);
    unsigned char* img = (unsigned char*)(pImage + step*seed.y);
    unsigned char* pMask = ptr(*msk, 0) + maskStep + sizeof(unsigned char);
    unsigned char* mask = (unsigned char*)(pMask + maskStep*seed.y);
    int i, L, R;
    int area = 0;
    int XMin, XMax, YMin = seed.y, YMax = seed.y;
    int _8_connectivity = (flags & 255) == 8;
    int fixedRange = flags & (1<<16);
    int fillImage = (flags & (1<<17)) == 0;
    FFillSegment* buffer_end = vector_front(buffer) + vector_size(buffer), *head = vector_front(buffer), *tail = vector_front(buffer);

    L = R = seed.x;
    if(mask[L])
        return;

    mask[L] = newMaskVal;
    unsigned char val0 =  img[L];

    if(fixedRange)
    {
        while(!mask[R + 1] && validInterval(diff, img + (R+1), &val0 ))
            mask[++R] = newMaskVal;

        while( !mask[L - 1] && validInterval(diff, img + (L-1), &val0 ))
            mask[--L] = newMaskVal;
    }
    else
    {
        while(!mask[R + 1] && validInterval(diff, img + (R+1), img + R))
            mask[++R] = newMaskVal;

        while(!mask[L - 1] && validInterval(diff, img + (L-1), img + L))
            mask[--L] = newMaskVal;
    }

    XMax = R;
    XMin = L;

    tail->y = (unsigned short)(seed.y);
    tail->l = (unsigned short)(L);
    tail->r = (unsigned short)(R);
    tail->prevl = (unsigned short)(R+1);
    tail->prevr = (unsigned short)(R);
    tail->dir = (short)(UP);
    if(++tail == buffer_end)                    
    {                                             
        vector_resize(buffer, vector_size(buffer) * 3/2);
        tail = vector_front(buffer) + (tail - head);  
        head = vector_front(buffer);
        buffer_end = head + vector_size(buffer);
    }

    while(head != tail)
    {
        int k, YC, PL, PR, dir;
        --tail;
        YC = tail->y;
        L = tail->l;
        R = tail->r;
        PL = tail->prevl;
        PR = tail->prevr;
        dir = tail->dir;

        int data[][3] =
        {
            {-dir, L - _8_connectivity, R + _8_connectivity},
            {dir, L - _8_connectivity, PL - 1},
            {dir, PR + 1, R + _8_connectivity}
        };

        unsigned length = (unsigned)(R-L);

        if(region)
        {
            area += (int)length + 1;

            if(XMax < R) XMax = R;
            if(XMin > L) XMin = L;
            if(YMax < YC) YMax = YC;
            if(YMin > YC) YMin = YC;
        }

        for( k = 0; k < 3; k++ )
        {
            dir = data[k][0];
            img = (unsigned char*)(pImage + (YC + dir) * step);
            unsigned char* img1 = (unsigned char*)(pImage + YC * step);
            mask = (unsigned char*)(pMask + (YC + dir) * maskStep);
            int left = data[k][1];
            int right = data[k][2];

            if(fixedRange)
            {
                for( i = left; i <= right; i++ )
                {
                    if( !mask[i] && validInterval(diff, img + i, &val0 ))
                    {
                        int j = i;
                        mask[i] = newMaskVal;
                        while( !mask[--j] && validInterval(diff, img + j, &val0 ))
                            mask[j] = newMaskVal;

                        while( !mask[++i] && validInterval(diff, img + i, &val0 ))
                            mask[i] = newMaskVal;

                        tail->y = (unsigned short)(YC + dir);
                        tail->l = (unsigned short)(j+1);
                        tail->r = (unsigned short)(i-1);
                        tail->prevl = (unsigned short)(L);
                        tail->prevr = (unsigned short)(R);
                        tail->dir = (short)(-dir);
                        if(++tail == buffer_end)                    
                        {                                             
                            vector_resize(buffer, vector_size(buffer) * 3/2);
                            tail = vector_front(buffer) + (tail - head);  
                            head = vector_front(buffer);
                            buffer_end = head + vector_size(buffer);
                        }
                    }
                }
            }
            else if(!_8_connectivity)
            {
                for( i = left; i <= right; i++ )
                {
                    if( !mask[i] && validInterval(diff, img + i, img1 + i ))
                    {
                        int j = i;
                        mask[i] = newMaskVal;
                        while( !mask[--j] && validInterval(diff, img + j, img + (j+1) ))
                            mask[j] = newMaskVal;

                        while( !mask[++i] &&
                              (validInterval(diff, img + i, img + (i-1) ) ||
                               (validInterval(diff, img + i, img1 + i) && i <= R)))
                            mask[i] = newMaskVal;

                        tail->y = (unsigned short)(YC + dir);
                        tail->l = (unsigned short)(j+1);
                        tail->r = (unsigned short)(i-1);
                        tail->prevl = (unsigned short)(L);
                        tail->prevr = (unsigned short)(R);
                        tail->dir = (short)(-dir);
                        if(++tail == buffer_end)                    
                        {                                             
                            vector_resize(buffer, vector_size(buffer) * 3/2);
                            tail = vector_front(buffer) + (tail - head);  
                            head = vector_front(buffer);
                            buffer_end = head + vector_size(buffer);
                        }

                    }
                }
            }
            else
            {
                for(i = left; i <= right; i++)
                {
                    int idx;
                    unsigned char val;

                    if(!mask[i] &&
                       (((val = img[i],
                          (unsigned)(idx = i-L-1) <= length) &&
                         validInterval(diff, &val, img1 + (i-1))) ||
                        ((unsigned)(++idx) <= length &&
                         validInterval(diff, &val, img1 + i )) ||
                        ((unsigned)(++idx) <= length &&
                         validInterval(diff, &val, img1 + (i+1)))))
                    {
                        int j = i;
                        mask[i] = newMaskVal;
                        while( !mask[--j] && validInterval(diff, img+j, img+(j+1)))
                            mask[j] = newMaskVal;

                        while( !mask[++i] &&
                              ((val = img[i],
                                validInterval(diff, &val, img + (i-1) )) ||
                               (((unsigned)(idx = i-L-1) <= length &&
                                 validInterval(diff, &val, img1 + (i-1) ))) ||
                               ((unsigned)(++idx) <= length &&
                                validInterval(diff &val, img1 + i )) ||
                               ((unsigned)(++idx) <= length &&
                                validInterval(diff, &val, img1 + (i+1) ))))
                            mask[i] = newMaskVal;

                        tail->y = (unsigned short)(YC + dir);
                        tail->l = (unsigned short)(j+1);
                        tail->r = (unsigned short)(i-1);
                        tail->prevl = (unsigned short)(L);
                        tail->prevr = (unsigned short)(R);
                        tail->dir = (short)(-dir);
                        if(++tail == buffer_end)                    
                        {                                             
                            vector_resize(buffer, vector_size(buffer) * 3/2);
                            tail = vector_front(buffer) + (tail - head);  
                            head = vector_front(buffer);
                            buffer_end = head + vector_size(buffer);
                        }
                    }
                }
            }
        }

        img = (unsigned char*)(pImage + YC * step);
        if(fillImage) 
        {
            for(i = L;i <= R;i++ )
                img[i] = newVal;
        }
    }

    if(region)
    {
        region->pt = seed;
        region->label = (int)newMaskVal;
        region->area = area;
        region->rect.x = XMin;
        region->rect.y = YMin;
        region->rect.width = XMax - XMin + 1;
        region->rect.height = YMax - YMin + 1;   
    }
}

void floodFill(Mat* img, Mat* mask, Point seedPoint, Scalar newval, Rect* rect, Scalar loDiff, Scalar upDiff, int flags)
{
    ConnectedComp comp;
    vector* buffer = malloc(sizeof(vector)); //FFillSegment
    vector_init(buffer);

    if(rect)
        *rect = init_Rect(0, 0, 0, 0);

    int i, connectivity = flags & 255;

    unsigned char* nv_buf;
    unsigned char* ld_buf = malloc(sizeof(unsigned char) * 3);
    unsigned char* ud_buf = malloc(sizeof(unsigned char) * 3);
    int sz[] = {img->rows, img->cols};

    int type = type(*img);
    int depth = depth(*img);
    int cn = channels(*img);

    if(connectivity == 0)
        connectivity = 4;

    bool is_simple = empty(mask) && (flags & (1<<17)) == 0;

    for(i = 0; i < cn; i++)
    {
        is_simple = is_simple && fabs(loDiff.val[i]) < DBL_EPSILON && fabs(upDiff.val[i]) < DBL_EPSILON;
    }

    nv_buf = malloc(sizeof(unsigned char) * cn);
    scalarToRawData(newVal, nv_buf, type, 0);
    int buffer_size = max(sz[0], sz[1]) * 2;
    vector_resize(buffer, buffer_size);

    if(is_simple)
    {
        size_t elem_size = (img->step)[1];
        const unsigned char* seed_ptr = ptr(*img, 0) + elem_size*seedPoint.x;

        size_t k = 0;
        for(; k < elem_size; k++) {
            if (seed_ptr[k] != nv_buf[k])
                break;
        }

        if( k != elem_size )
        {
            floodFill_CnIR(img, seedPoint, nv_buf[0], &comp, flags, buffer);
            if(rect)
                *rect = comp.rect;
            return comp.area;
        }
    }

    if(empty(*mask))
    {
        Mat* tempMask = malloc(sizeof(Mat));
        create(tempMask, sz[0]+2, sz[1]+2, 0);
        setTo(tempMask, init_Scalar(0, 0, 0, 0));
        mask = tempMask;
    }
    else
    {
        assert(mask->rows == sz[0]+2 && mask->cols == sz[1]+2 );
        assert(type(*mask) == 0);
    }

    memset(ptr(mask, 0), 1, mask->cols);
    memset(ptr(mask, mask->rows-1), 1, mask->cols);

    for(i = 1; i < sz[0]; i++)
    {
        ((unsigned char*)(mask->data + mask->step[0] * i))[0] = 
            ((unsigned char*)(mask->data + (mask->step)[0] * i))[mask->cols-1] = 
                    (unsigned char)1;
    }

    for( i = 0; i < cn; i++ )
    {
        ld_buf[i] = (unsigned char)floor(loDiff.val[i]);
        ud_buf[i] = (unsigned char)floor(upDiff.val[i]);
    }

    unsigned char newMaskVal = (unsigned char)((flags & 0xff00) == 0 ? 1 : ((flags >> 8) & 255));

    Diff8uC1* diff = malloc(sizeof(Diff8uC1));
    diff8uC1(obj, ld_buf[0], ud_buf[0]);
    floodFillGrad_CnIR(img, mask, seedPoint, nv_buf[0], newMaskVal, obj,
            &comp, flags, buffer);
    if(rect)
       *rect = comp.rect;
    return comp.area;
}