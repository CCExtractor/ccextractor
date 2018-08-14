#include "color.h"

void rangeop_(CvtColorLoop_Invoker* body, Point range, int code)
{
    const unsigned char* yS = body->src_data + (size_t)(range.x)*body->src_step;
    unsigned char* yD = body->dst_data + (size_t)(range.x)*body->dst_step;

    for(int i = range.x; i < range.y; ++i, yS += src_step, yD += dst_step)
    {
        if(code == 0)
            RGB2gray((RGBtoGray*)(body->cvt), yS, yD, width);

        if(code == 1)
            RGB2Hls_b((RGB2HLS_b*)(body->cvt), yS, yD, width);

        if(code == 2)
            RGB2lab_b((RGB2Lab_b*)(body->cvt), yS, yD, width);
    }
}

void parallel_for__(Point range, CvtColorLoop_Invoker body, double nstripes, int code)
{
    (void)nstripes;
    rangeop_(&body, range, code);
}

void CvtColorLoop(const unsigned char* src_data, size_t src_step, unsigned char* dst_data, size_t dst_step, int width, int height, void* cvt, int code)
{
    parallel_for__(init_Point(0, height),
                  cvtColorLoop_Invoker(src_data, src_step, dst_data, dst_step, width, cvt),
                  (width * height) / (double)(1<<16), code);
}

void RGBtoLab_b(RGB2Lab_b* r2l, int _srccn, int blueIdx, const float* _coeffs,
              const float* _whitept, bool _srgb)
{
    r2l->srccn = _srccn;
    r2l->srgb = _srgb;
    static volatile int _3 = 3;

    uint64_t whitePt[] = {4606736166120140520, 4607182418800017408, 4607582131281345049};
    static const uint64_t lshift = 4661225614328463360;

    uint64_t sRGB2XYZ_D65[] = {4601101712626337293, 4600113208536926488, 4595668443935087960,
                               4596830300581355510, 4604616808164296984, 4589864745167288149,
                               4581229867500941131, 4593253181469327672, 4606734103471511185};
    for( int i = 0; i < _3; i++ )
    {
        uint64_t c[3];
        for(int j = 0; j < 3; j++)
            c[j] = sRGB2XYZ_D65[i*3+j];

        r2l->coeffs[i*3+(blueIdx^2)] = round(lshift*c[0]/whitePt[i]);
        r2l->coeffs[i*3+1]           = round(lshift*c[1]/whitePt[i]);
        r2l->coeffs[i*3+blueIdx]     = round(lshift*c[2]/whitePt[i]);
    }
}


void cvtBGRtoLab(const unsigned char * src_data, size_t src_step,
                 unsigned char* dst_data, size_t dst_step,
                 int width, int height,
                 int depth, int scn, bool swapBlue, bool isLab, bool srgb)
{
    int blueIdx = swapBlue ? 2 : 0;

    RGB2Lab_b r2l;
    RGBtoLab_b(&r2l, scn, blueIdx, 0, 0, srgb)
    CvtColorLoop(src_data, src_step, dst_data, dst_step, width, height, &r2l, 2);
}

void cvtColorBGR2Lab(Mat _src, Mat* _dst, bool swapb, bool srgb)
{
    CvtHelper h = cvtHelper(_src, _dst, 3);

    cvtBGRtoLab(h.src.data, h.src.step, h.dst.data, h.dst.step, h.src.cols, h.src.rows,
                     h.depth, h.scn, swapb, true, srgb);
}

void cvtBGRtoHSV(const unsigned char* src_data, size_t src_step,
                 unsigned char * dst_data, size_t dst_step,
                 int width, int height,
                 int depth, int scn, bool swapBlue, bool isFullRange, bool isHSV)
{
    int hrange = depth == 5 ? 360 : isFullRange ? 256 : 180;
    int blueIdx = swapBlue ? 2 : 0;
    RGB2HLS_b r2h = RGB2HLSb(scn, blueIdx, hrange);
    CvtColorLoop(src_data, src_step, dst_data, dst_step, width, height, &r2h, 1);
}

void cvtColorBGR2HLS(Mat _src, Mat* _dst, bool swapb, bool fullRange)
{
    CvtHelper h = cvtHelper(_src, _dst, 3);

    cvtBGRtoHSV(h.src.data, h.src.step, h.dst.data, h.dst.step, h.src.cols, h.src.rows,
                h.depth, h.scn, swapb, fullRange, false);
}

void CvtBGRtoGray(unsigned char* src_data, size_t src_step,
                             unsigned char * dst_data, size_t dst_step,
                             int width, int height,
                             int depth, int scn, bool swapBlue);
{
    int blueIdx = swapBlue ? 2 : 0;
    RGBtoGray r2g = RGB2Gray(scn, blueIdx, 0);
    CvtColorLoop(src_data, src_step, dst_data, dst_step, width, height, &r2g, 0);
}

void cvtColorBGR2Gray(Mat _src, Mat *_dst, bool swapb)
{
    CvtHelper h = cvtHelper(_src, _dst, 1);
    CvtBGRtoGray(h.src.data, h.src.step[0], h.dst.data, h.dst.step[0], h.src.cols, h.src.rows,
                      h.depth, h.scn, swapb);
}

void RGB2lab_b(RGB2Lab_b* r2l, const unsigned char* src, unsigned char* dst, int n)
{
    const int Lscale = (116*255+50)/100;
    const int Lshift = -((16*255*(1 << 15) + 50)/100);
    const unsigned short tab[] = {0, 1, 1, 2, 2, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 10, 11, 11, 12, 13, 14, 15, 16, 17, 19, 20, 21, 22, 24, 25, 26, 28, 29, 31, 33, 34, 36, 38, 40, 41, 43, 45, 47, 49, 51, 54, 56, 58, 60, 63, 65, 68, 70, 73, 75, 78, 81, 83, 86, 89, 92, 95, 98, 101, 105, 108, 111, 115, 118, 121, 125, 129, 132, 136, 140, 144, 147, 151, 155, 160, 164, 168, 172, 176, 181, 185, 190, 194, 199, 204, 209, 213, 218, 223, 228, 233, 239, 244, 249, 255, 260, 265, 271, 277, 282, 288, 294, 300, 306, 312, 318, 324, 331, 337, 343, 350, 356, 363, 370, 376, 383, 390, 397, 404, 411, 418, 426, 433, 440, 448, 455, 463, 471, 478, 486, 494, 502, 510, 518, 527, 535, 543, 552, 560, 569, 578, 586, 595, 604, 613, 622, 631, 641, 650, 659, 669, 678, 688, 698, 707, 717, 727, 737, 747, 757, 768, 778, 788, 799, 809, 820, 831, 842, 852, 863, 875, 886, 897, 908, 920, 931, 943, 954, 966, 978, 990, 1002, 1014, 1026, 1038, 1050, 1063, 1075, 1088, 1101, 1113, 1126, 1139, 1152, 1165, 1178, 1192, 1205, 1218, 1232, 1245, 1259, 1273, 1287, 1301, 1315, 1329, 1343, 1357, 1372, 1386, 1401, 1415, 1430, 1445, 1460, 1475, 1490, 1505, 1521, 1536, 1551, 1567, 1583, 1598, 1614, 1630, 1646, 1662, 1678, 1695, 1711, 1728, 1744, 1761, 1778, 1794, 1811, 1828, 1846, 1863, 1880, 1897, 1915, 1933, 1950, 1968, 1986, 2004, 2022, 2040};
    const unsigned short LabCbrtTab_b[] = {4520, 4645, 4770, 4895, 5020, 5145, 5270, 5395, 5520, 5645, 5771, 5896, 6021, 6146, 6271, 6396, 6521, 6646, 6771, 6894, 7013, 7128, 7240, 7348, 7453, 7555, 7654, 7751, 7846, 7938, 8028, 8116, 8203, 8287, 8370, 8451, 8531, 8609, 8686, 8762, 8836, 8909, 8981, 9052, 9121, 9190, 9257, 9324};
    int i, scn = r2l->srccn;
    int C0 = r2l->coeffs[0], C1 = r2l->coeffs[1], C2 = r2l->coeffs[2],
        C3 = r2l->coeffs[3], C4 = r2l->coeffs[4], C5 = r2l->coeffs[5],
        C6 = r2l->coeffs[6], C7 = r2l->coeffs[7], C8 = r2l->coeffs[8];
    n *= 3;

    i = 0;
    for(; i < n; i += 3, src += scn)
    {
        int R = tab[src[0]], G = tab[src[1]], B = tab[src[2]];
        int fX = LabCbrtTab_b[CV_DESCALE(R*C0 + G*C1 + B*C2, lab_shift)];
        int fY = LabCbrtTab_b[CV_DESCALE(R*C3 + G*C4 + B*C5, lab_shift)];
        int fZ = LabCbrtTab_b[CV_DESCALE(R*C6 + G*C7 + B*C8, lab_shift)];

        int L = CV_DESCALE(Lscale*fY + Lshift, lab_shift2);
        int a = CV_DESCALE(500*(fX - fY) + 128*(1 << lab_shift2), lab_shift2);
        int b = CV_DESCALE(200*(fY - fZ) + 128*(1 << lab_shift2), lab_shift2);

        dst[i] = (unsigned char)L;
        dst[i+1] = (unsigned char)a;
        dst[i+2] = (unsigned char)b;
    }
}

void cvtColor(Mat src, Mat* dst, int code)
{
    if(code == 0)
        cvtColorBGR2Gray(src, dst, true);

    if(code == 1)
        cvtColorBGR2HLS(src, dst, true, false);

    if(code == 2)
        cvtColorBGR2Lab(src, dst, true, true);
}