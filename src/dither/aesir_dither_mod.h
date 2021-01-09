#pragma once

// A common header file to be used by all mods
#include "aesir_mod.h"

// Exports functions used by the aesir dither plugin
// That may need to be tweaked, and the .dll they are contained in reloaded
// Ideally, there should be no state, and everything is driven through a
// structure with function pointers, returned by an exported DLL function.

#define AESIR_DITHER_MOD_FUNC_LIST_(_) \
    _(int, sum, (int a, int b)) \
    _(int, avg, (int a, int b))

struct aesir_dither_mod_functions
{
    AESIR_DITHER_MOD_FUNC_LIST_(AESIR_MOD_FUNC_DECL_)
};

#define AESIR_DITHER_MOD_LOAD() (struct aesir_dither_mod_functions*)(aesir_mod_load())
