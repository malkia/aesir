#include "aesir_dither.h"

typedef struct {
	A_u_long	index;
	A_char		str[256];
} TableString;

TableString	g_strs[StrID_NUMTYPES] = {
	StrID_NONE,						"",
	StrID_Name,						"aesir_dither",
	StrID_Description,				"aesir_dither skeleton\rhttps://github.com/malkia/aesir",
	StrID_Gain_Param_Name,			"Gain",
	StrID_Color_Param_Name,			"Color",
};

char *GetStringPtr(int strNum)
{
	return g_strs[strNum].str;
}
	