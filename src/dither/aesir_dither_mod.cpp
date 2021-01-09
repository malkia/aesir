#include "aesir_dither_mod.h"

static int sum(int a, int b)
{
    return a + b; 
}

static int avg(int a, int b)
{
    return (a + b) / 2; 
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
