#pragma once

// Mods are poor-man hack to support C/C++ live coding, allowing fast iteration developing plugins.
// They allow part of the code to be recompiled while the plugin is still working in the host.
// 
// While there are several notable solutions to the problem, including Visual Studio's Edit & Continue,
// these either failed for me, or I failed evaluating them properly. This needs to be revisited again.
// 
// In it's current implementation, A mod is a .dll with one exported function, returning a pointer to a
// well known structure of function pointers. It's expected that the user calls the exported function
// frequently enough, that it checks for newer versions of the "mod" and if that's the case reloads it.
//
// Specifically on NTFS, for a plugin named "fancy_plugin.dll" the mod would be copied in an unique
// NTFS stream, named "fancy_plugin.dll:fancy_plugin_mod.dll.vXXX" - e.g. the mod would be in a way
// part of the plugin, hence if the plugin needs to be recompiled it'll trigger an error, requiring the
// host to be shutdown, so that the NTFS stream files can be deleted.

#define AESIR_MOD_FUNC_DECL_(r,f,a) r(*f)a;

void* aesir_mod_load();
