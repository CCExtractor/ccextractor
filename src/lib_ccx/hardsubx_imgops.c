#include "lib_ccx.h"
#include "utility.h"

#ifdef ENABLE_OCR
//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "allheaders.h"
#include "hardsubx.h"

#define BLACK 20.0
#define YELLOW 70.0

void rgb2lab(float R, float G, float B,float *L, float *a, float *b)
{
	//Conversion to the CIE-LAB color space to get the Luminance
	float X, Y, Z, fX, fY, fZ;

	X = 0.412453*R + 0.357580*G + 0.180423*B;
	Y = 0.212671*R + 0.715160*G + 0.072169*B;
	Z = 0.019334*R + 0.119193*G + 0.950227*B;

	X /= (255 * 0.950456);
	Y /=  255;
	Z /= (255 * 1.088754);

	if (Y > 0.008856)
	{
		fY = pow(Y, 1.0/3.0);
		*L = 116.0*fY - 16.0;
	}
	else
	{
		fY = 7.787*Y + 16.0/116.0;
		*L = 903.3*Y;
	}

	if (X > 0.008856)
		fX = pow(X, 1.0/3.0);
	else
		fX = 7.787*X + 16.0/116.0;

	if (Z > 0.008856)
		fZ = pow(Z, 1.0/3.0);
	else
		fZ = 7.787*Z + 16.0/116.0;

	*a = 500.0*(fX - fY);
	*b = 200.0*(fY - fZ);

	if (*L < BLACK) {
	*a *= exp((*L - BLACK) / (BLACK / 4));
	*b *= exp((*L - BLACK) / (BLACK / 4));
	*L = BLACK;
	}
	if (*b > YELLOW)
	*b = YELLOW;
}

#endif