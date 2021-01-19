#pragma once

// A common header file to be used by all mods
#include "aesir_mod.h"

#include <AE_Effect.h>

// Exports functions used by the aesir dither plugin
// That may need to be tweaked, and the .dll they are contained in reloaded
// Ideally, there should be no state, and everything is driven through a
// structure with function pointers, returned by an exported DLL function.

#define AESIR_DITHER_MOD_FUNC_LIST_(_) \
	_(PF_Err, pixel8, (void* refcon, A_long xL, A_long yL, PF_Pixel8* inP, PF_Pixel8*outP))

struct aesir_dither_mod_functions
{
    AESIR_DITHER_MOD_FUNC_LIST_(AESIR_MOD_FUNC_DECL_)
};

#define AESIR_DITHER_MOD_LOAD() (struct aesir_dither_mod_functions*)(aesir_mod_load())
