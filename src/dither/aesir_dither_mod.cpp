#include "aesir_dither_mod.h"

static uint8_t c64dither[8][8] = {
     0, 32,  8, 40,  2, 32, 10, 42,
	48, 16, 56, 24, 50, 18, 58, 26,
	12, 44,	 4, 36, 14, 46,  6, 38,
	60, 28, 52,	20, 62, 30, 54, 22,
	 3, 35, 11, 43,  1, 33,  9, 41,
	51, 19, 59, 27, 49,	17, 57, 25,
	15, 47, 7, 39, 13, 45,   5, 37,
	63, 31, 55, 23, 61, 29, 53, 21,
};

static PF_Err pixel8(void* refcon, A_long xL, A_long yL, PF_Pixel8* inP, PF_Pixel8*outP)
{
	int dx = xL & 7;
	int dy = yL & 7;
	uint32_t vr = (c64dither[dy][dx]+1) * 4 - 1;
	uint32_t vg = (c64dither[dy][7-dx]+1) * 4 - 1;
	uint32_t vb = (c64dither[7-dy][dx]+1) * 4 - 1;
	uint32_t r = inP->red;
	uint32_t g = inP->green;
	uint32_t b = inP->blue;
	outP->alpha = inP->alpha;
	outP->red = ((r*vr) >> 8) | (g<vb?0:0x80);
	outP->green = ((g*vg) >> 8) | (b<vr?0:0x80);;
	outP->blue = ((b*vb) >> 8) | (g<vg?0:0x80);;
	outP->blue = 255;
	return PF_Err_NONE;
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
