#include "lib_ccx.h"
#include "utility.h"

#ifdef ENABLE_HARDSUBX
//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <leptonica/allheaders.h>
#include "hardsubx.h"

#define BLACK 20.0
#define YELLOW 70.0

#define min_f(a, b, c) (fminf(a, fminf(b, c)))
#define max_f(a, b, c) (fmaxf(a, fmaxf(b, c)))

void rgb_to_hsv(float R, float G, float B, float *H, float *S, float *V)
{
	//Conversion into HSV color space to get Hue
	float r = R / 255.0f;
	float g = G / 255.0f;
	float b = B / 255.0f;

	float h, s, v; // h:0-360.0, s:0.0-1.0, v:0.0-1.0

	float max = max_f(r, g, b);
	float min = min_f(r, g, b);

	v = max;

	if (max == 0.0f)
	{
		s = 0;
		h = 0;
	}
	else if (max - min == 0.0f)
	{
		s = 0;
		h = 0;
	}
	else
	{
		s = (max - min) / max;

		if (max == r)
		{
			h = 60 * ((g - b) / (max - min)) + 0;
		}
		else if (max == g)
		{
			h = 60 * ((b - r) / (max - min)) + 120;
		}
		else
		{
			h = 60 * ((r - g) / (max - min)) + 240;
		}
	}

	if (h < 0)
		h += 360.0f;

	*H = (unsigned char)(h);       // dst_h : 0-360
	*S = (unsigned char)(s * 255); // dst_s : 0-255
	*V = (unsigned char)(v * 255); // dst_v : 0-255
}

void rgb_to_lab(float R, float G, float B, float *L, float *a, float *b)
{
	//Conversion to the CIE-LAB color space to get the Luminance
	float X, Y, Z, fX, fY, fZ;

	X = 0.412453 * R + 0.357580 * G + 0.180423 * B;
	Y = 0.212671 * R + 0.715160 * G + 0.072169 * B;
	Z = 0.019334 * R + 0.119193 * G + 0.950227 * B;

	X /= (255 * 0.950456);
	Y /= 255;
	Z /= (255 * 1.088754);

	if (Y > 0.008856)
	{
		fY = pow(Y, 1.0 / 3.0);
		*L = 116.0 * fY - 16.0;
	}
	else
	{
		fY = 7.787 * Y + 16.0 / 116.0;
		*L = 903.3 * Y;
	}

	if (X > 0.008856)
		fX = pow(X, 1.0 / 3.0);
	else
		fX = 7.787 * X + 16.0 / 116.0;

	if (Z > 0.008856)
		fZ = pow(Z, 1.0 / 3.0);
	else
		fZ = 7.787 * Z + 16.0 / 116.0;

	*a = 500.0 * (fX - fY);
	*b = 200.0 * (fY - fZ);

	if (*L < BLACK)
	{
		*a *= exp((*L - BLACK) / (BLACK / 4));
		*b *= exp((*L - BLACK) / (BLACK / 4));
		*L = BLACK;
	}
	if (*b > YELLOW)
		*b = YELLOW;
}

#endif
