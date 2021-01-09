#include "aesir_dither.h"

#include <string>

#include "aesir_dither_mod.h"

static void* dither_mod_functions = nullptr;

static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	suites.ANSICallbacksSuite1()->sprintf(	out_data->return_msg,
											"%s v%d.%d\r%s",
											STR(StrID_Name), 
											MAJOR_VERSION, 
											MINOR_VERSION, 
											STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err 
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	out_data->my_version = PF_VERSION(	MAJOR_VERSION, 
										MINOR_VERSION,
										BUG_VERSION, 
										STAGE_VERSION, 
										BUILD_VERSION);

	out_data->out_flags =  PF_OutFlag_DEEP_COLOR_AWARE;	// just 16bpc, not 32bpc
	
	return PF_Err_NONE;
}

static PF_Err 
ParamsSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err		err		= PF_Err_NONE;
	PF_ParamDef	def;	

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(	STR(StrID_Gain_Param_Name), 
							SKELETON_GAIN_MIN, 
							SKELETON_GAIN_MAX, 
							SKELETON_GAIN_MIN, 
							SKELETON_GAIN_MAX, 
							SKELETON_GAIN_DFLT,
							PF_Precision_HUNDREDTHS,
							0,
							0,
							GAIN_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_COLOR(	STR(StrID_Color_Param_Name), 
					PF_HALF_CHAN8,
					PF_MAX_CHAN8,
					PF_MAX_CHAN8,
					COLOR_DISK_ID);
	
	out_data->num_params = SKELETON_NUM_PARAMS;

	return err;
}

static PF_Err
MySimpleGainFunc16 (
	void		*refcon, 
	A_long		xL, 
	A_long		yL, 
	PF_Pixel16	*inP, 
	PF_Pixel16	*outP)
{
	PF_Err		err = PF_Err_NONE;

	GainInfo	*giP	= reinterpret_cast<GainInfo*>(refcon);
	PF_FpLong	tempF	= 0;
					
	if (giP){
		tempF = giP->gainF * PF_MAX_CHAN16 / 100.0;
		if (tempF > PF_MAX_CHAN16){
			tempF = PF_MAX_CHAN16;
		};

		outP->alpha		=	inP->alpha;
		outP->red		=	MIN((inP->red	+ (A_u_char) tempF), PF_MAX_CHAN16);
		outP->green		=	MIN((inP->green	+ (A_u_char) tempF), PF_MAX_CHAN16);
		outP->blue		=	MIN((inP->blue	+ (A_u_char) tempF), PF_MAX_CHAN16);
	}

	return err;
}

// https://github.com/jonmortiboy/dither/blob/master/dither.c


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

static PF_Err
MySimpleGainFunc8 (
	void		*refcon, 
	A_long		xL, 
	A_long		yL, 
	PF_Pixel8	*inP, 
	PF_Pixel8	*outP)
{
	PF_Err		err = PF_Err_NONE;

	GainInfo	*giP	= reinterpret_cast<GainInfo*>(refcon);
	if( !giP )
		return err;
	
	PF_FpLong	tempF	= 0;
					
	tempF = giP->gainF * PF_MAX_CHAN8 / 100.0;
	if (tempF > PF_MAX_CHAN8){
		tempF = PF_MAX_CHAN8;
	};

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

	return err;
}

static PF_Err 
Render (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err				err		= PF_Err_NONE;
	AEGP_SuiteHandler	suites(in_data->pica_basicP);

	/*	Put interesting code here. */
	GainInfo			giP;
	AEFX_CLR_STRUCT(giP);
	A_long				linesL	= 0;

	linesL 		= output->extent_hint.bottom - output->extent_hint.top;
	giP.gainF 	= params[SKELETON_GAIN]->u.fs_d.value;
	
	if (PF_WORLD_IS_DEEP(output)){
		ERR(suites.Iterate16Suite1()->iterate(	in_data,
												0,								// progress base
												linesL,							// progress final
												&params[SKELETON_INPUT]->u.ld,	// src 
												NULL,							// area - null for all pixels
												(void*)&giP,					// refcon - your custom data pointer
												MySimpleGainFunc16,				// pixel function pointer
												output));
	} else {
		ERR(suites.Iterate8Suite1()->iterate(	in_data,
												0,								// progress base
												linesL,							// progress final
												&params[SKELETON_INPUT]->u.ld,	// src 
												NULL,							// area - null for all pixels
												(void*)&giP,					// refcon - your custom data pointer
												MySimpleGainFunc8,				// pixel function pointer
												output));	
	}

	return err;
}


extern "C" DllExport
PF_Err PluginDataEntryFunction(
	PF_PluginDataPtr inPtr,
	PF_PluginDataCB inPluginDataCallBackPtr,
	SPBasicSuite* inSPBasicSuitePtr,
	const char* inHostName,
	const char* inHostVersion)
{
	PF_Err result = PF_Err_INVALID_CALLBACK;

	result = PF_REGISTER_EFFECT(
		inPtr,
		inPluginDataCallBackPtr,
		"00_aesir_dither", // Name
		"aesir dither", // Match Name
		"aesir", // Category
		AE_RESERVED_INFO); // Reserved Info

	return result;
}

static std::wstring aesir_win32_mod_filename;
static HMODULE aesir_win32_mod_handle[4096];
static bool aesir_win32_mod_reload()
{
	// LoadLibraryExW();
}
static bool aesir_win32_mod_init()
{
	// HMODULE thisModule = nullptr;
	// aesir_win32_mod_filename.reserve(32768);
	// GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&aesir_win32_mod_init, &thisModule );
	// GetModuleFileNameW(thisModule, aesir_win32_mod_filename.data(), sizeof(aesir_win32_mod_filename)/sizeof(wchar_t));
	// _wcslwr(aesir_win32_mod_filename);
	// auto len = wcslen(aesir_win32_mod_filename);
	// auto extOffset =len - 4;
	// if( wcscmp(L".aex", aesir_win32_mod_filename + extOffset) !=0 )
	// 	return false;
	// wcscat(aesir_win32_mod_filename, L"_mod.mod");
}

static int cnt = 0;

PF_Err
EffectMain(
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;

	const auto functions = AESIR_DITHER_MOD_LOAD();
	if( functions && dither_mod_functions != functions )
	{
		printf( "AESIR_DITHER: New functions %p\n", functions );
		dither_mod_functions = functions;
	}

	printf("AESIR_DITHER: Loaded... %d, functions=%p, sum(13,15)=%d\n", cnt++, functions, functions ? functions->sum(13, 15) : -1);

	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:

				err = About(in_data,
							out_data,
							params,
							output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:

				err = GlobalSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:

				err = ParamsSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_RENDER:

				err = Render(	in_data,
								out_data,
								params,
								output);
				break;
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}

