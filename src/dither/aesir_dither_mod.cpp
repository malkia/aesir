#include "aesir_dither_mod.h"

#include <AE_Macros.h>

typedef struct RGB {
	int r;
	int g;
	int b;
} RGB;

RGB difRGB(RGB from, RGB to) {
	RGB dif;
	dif.r = to.r - from.r;
	dif.g = to.g - from.g;
	dif.b = to.b - from.b;
	
	return dif;
}

RGB addRGB(RGB a, RGB b) {
	RGB sum;
	sum.r = a.r + b.r;
	sum.g = a.g + b.g;
	sum.b = a.b + b.b;
	
	if (sum.r > 255) sum.r = 255; if (sum.r < 0) sum.r = 0;
	if (sum.g > 255) sum.g = 255; if (sum.g < 0) sum.g = 0;
	if (sum.b > 255) sum.b = 255; if (sum.b < 0) sum.b = 0;
	
	return sum;
}

RGB divRGB(RGB rgb, double d) {
	RGB div;
	div.r = (int)((double)rgb.r/d);
	div.g = (int)((double)rgb.g/d);
	div.b = (int)((double)rgb.b/d);
	
	return div;
}

RGB mulRGB(RGB rgb, double d) {
	RGB mul;
	mul.r = (int)((double)rgb.r*d);
	mul.g = (int)((double)rgb.g*d);
	mul.b = (int)((double)rgb.b*d);
	
	return mul;
}

double distRGB(RGB from, RGB to) {
	RGB dif = difRGB(from, to);
	double dist = dif.r*dif.r + dif.g*dif.g + dif.b*dif.b;
	
	return dist;
}

RGB nearestRGB(RGB rgb, RGB rgbs[], int numRGBs) {
	double dist = -1, tempDist;
	RGB nearest;
	
	int i;
	for (i = 0; i < numRGBs; i++) {
		tempDist = distRGB(rgb, rgbs[i]);
		
		if (tempDist < dist || dist < 0) {
			dist = tempDist;
			nearest = rgbs[i];
		}
	}
	
	return nearest;
}

// Define the 4bit colour palette
int numCols = 16;
RGB cols4bit[] = { 	{0,0,0}, {128,0,0}, {255,0,0}, {255,0,255},
					{0,128,128}, {0,128,0}, {0,255,0}, {0,255,255},
					{0,0,128}, {128,0,128}, {0,0,255}, {192,192,192},
				 	{128,128,128}, {128,128,0}, {255,255,0}, {255,255,255} };

int  aryOrderedDither[3][3] = {{ 28,255, 57},
                               {142,113,227},
                               {170,198, 85}};

int bayer[8][8] = { 1,  49, 13, 61, 4,  52, 16, 64,
					33, 17, 45, 29, 36, 20, 48, 32,
					9,  57, 5,  53, 12, 60, 8,  56,
					41, 25, 37, 21, 44, 28, 40, 24,
					3,  51, 15, 63, 2,  50, 14, 62,
					35, 19, 47, 31, 34, 18, 46, 30,
					11, 59, 7,  55, 10, 58, 6,  54,
					43, 27, 39, 23, 42, 26, 38, 22 };

int bayerHalftone[8][8] = 
				{ 24, 10, 12, 26, 35, 47, 49, 37,
					8,  0,  2,  14, 45, 59, 61, 51,
					22, 6,  4,  16, 43, 57, 63, 53,
					30, 20, 18, 28, 33, 41, 55, 39,
					34, 46, 48, 36, 25, 11, 13, 27,
					44, 58, 60, 50, 9,  1,  3,  15,
					42, 56, 62, 52, 23, 7,  5,  17,
					32, 40, 54, 38, 31, 21, 19, 29 };

static PF_Err pixel8(void* refcon, A_long xL, A_long yL, PF_Pixel8* inP, PF_Pixel8*outP)
{
	PF_Err		err = PF_Err_NONE;


	PF_FpLong	tempF	= 0;
					
	outP->alpha		=	inP->alpha;
	outP->red		=	MIN((inP->red	+ (A_u_char) tempF), PF_MAX_CHAN8);
	outP->green		=	MIN((inP->green	+ (A_u_char) tempF), PF_MAX_CHAN8);
	outP->blue		=	MIN((inP->blue	+ (A_u_char) tempF), PF_MAX_CHAN8);


	int dx = xL % 8;
	int dy = yL % 8;

	RGB rgb;
	
	rgb.r = inP->red / 17;
	rgb.g = inP->green / 17;
	rgb.b = inP->blue / 17;

	int dither = bayer[dy][dx];

	rgb.r *= dither;
	rgb.g *= dither;
	rgb.b *= dither;
	
	RGB n = nearestRGB(rgb, cols4bit, numCols);

	outP->red = (n.r * inP->red) / 255;
	outP->green = (n.g * inP->green) / 255;
	outP->blue = (n.b *inP->blue) / 255;

	//outP->red = 0;
	//outP->blue = 0;
	outP->green = 255;

	return err;
}

extern "C" __declspec(dllexport) const struct aesir_dither_mod_functions* aesir_mod_functions()
{
    static const struct aesir_dither_mod_functions functions = {
        #define AESIR_MOD_FUNC_NAME_(r,f,a) f,
        AESIR_DITHER_MOD_FUNC_LIST_(AESIR_MOD_FUNC_NAME_)
        #undef AESIR_MOD_FUNC_NAME_
    };
    return &functions;
}
