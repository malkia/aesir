#pragma optimize("", off)

#include "aesir_mod.h"

#include <string>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN 1
#define WIN32_EXTRA_LEAN 1
#include <windows.h>

static WIN32_FILE_ATTRIBUTE_DATA aesir_mod_source_fileattr;
static std::wstring              aesir_mod_source_filename;
static void*                     aesir_mod_shadow_functions; // cached functions
static std::wstring              aesir_mod_shadow_filename;
static int                       aesir_mod_shadow_version;
static const std::wstring		 aesir_mod_tmp_pattern(L".tmp.00000");

static std::wstring aesir_win32_module_filename( HMODULE module )
{
	std::wstring name;
	DWORD nameLength = MAX_PATH;
	for( ;; ) {
		name.resize(nameLength + 1);
		auto returned = GetModuleFileNameW(module, const_cast<wchar_t*>(name.data()), nameLength);
		if( returned < nameLength )
		{
			name.resize(returned);
			break;
		}
		// GetModuleFileNameW does not return the exact count, so we have to double here.
		nameLength *= 2;
	}
	printf("AESIR_MOD: GetModuleFileNameW(%p, ...) returned \"%S\"\n", module, name.c_str());
	return name;
}

static std::wstring aesir_win32_resolve_filename( const std::wstring& fileName )
{
	std::wstring name;
	auto h = CreateFileW(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if( h == INVALID_HANDLE_VALUE )
	{
		printf("AESIR_MOD: CreateFileW(\"%S\", ...) failed with 0x%08.8X\n", fileName.c_str(), GetLastError());
		return name;
	}
	DWORD nameLength = MAX_PATH;
	for( ;; ) {
		name.resize(nameLength + 1);
		auto returned = GetFinalPathNameByHandleW(h, const_cast<wchar_t*>(name.data()), nameLength, 0 );
		if( returned < nameLength )
		{
			name.resize(returned);
			break;
		}
		// GetFilePathByHandleW returns the exact count needed
		nameLength = returned;
	}
	CloseHandle(h);
	printf("AESIR_MOD: GetFinalPathNameByHandleW(%p, ...) returned \"%S\"\n", h, name.c_str());
	return name;
}

static bool aesir_mod_init()
{
	if( !aesir_mod_source_filename.empty() )
		return true;
	
	HMODULE thisModule = nullptr;
	// Get the name of the module this function is in, (this should be in the plugin).
	auto r = GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPWSTR)&aesir_mod_init, &thisModule );
	if( !r )
		return false;

	auto pluginName = aesir_win32_module_filename(thisModule);
	if( pluginName.empty() )
		return false;
	
	pluginName = aesir_win32_resolve_filename(pluginName);
	if( pluginName.empty() )
		return false;

	auto lastSlash = pluginName.find_last_of(L'\\');
	auto lastDot = pluginName.find_last_of(L'.');
	if( lastDot == std::wstring::npos || lastSlash == std::wstring::npos || lastDot <= lastSlash )
		return false;

	_wcslwr(const_cast<wchar_t*>(pluginName.data()));

	auto moduleName{ pluginName.substr( 0, lastDot ) + L"_mod.dll" };

    WIN32_FILE_ATTRIBUTE_DATA data;
    if( !GetFileAttributesExW(moduleName.c_str(), GetFileExInfoStandard, &data) )
    {
		printf("AESIR_MOD(Init): GetFileAttributesExW(\"%S\", ...) failed with %08.8X\n", moduleName.c_str(), GetLastError());
        return false;
	}

	aesir_mod_source_filename = moduleName;
    aesir_mod_shadow_filename = pluginName + aesir_mod_tmp_pattern;
	printf("AESIR_MOD(Init): Using module \"%S\"\n", moduleName.c_str() );
	return true;
}

void* aesir_mod_load()
{
    if( !aesir_mod_init() )
    {
		printf("AESIR_MOD(Load): Called without being initialized\n" );
		return nullptr;
	}
	
    WIN32_FILE_ATTRIBUTE_DATA fileattr;
    auto b = GetFileAttributesExW(aesir_mod_source_filename.c_str(), GetFileExInfoStandard, &fileattr);
    if( !b )
    {
		printf("AESIR_MOD(Load): GetFileAttributesExW(\"%S\", ...) failed with %08.8X\n", aesir_mod_source_filename.c_str(), GetLastError() );
        return nullptr;
    }

	if( fileattr.nFileSizeHigh == aesir_mod_source_fileattr.nFileSizeHigh &&
		fileattr.nFileSizeLow == aesir_mod_source_fileattr.nFileSizeLow &&
		fileattr.ftCreationTime.dwHighDateTime == aesir_mod_source_fileattr.ftCreationTime.dwHighDateTime &&
		fileattr.ftCreationTime.dwLowDateTime == aesir_mod_source_fileattr.ftCreationTime.dwLowDateTime &&
		fileattr.ftLastWriteTime.dwHighDateTime == aesir_mod_source_fileattr.ftLastWriteTime.dwHighDateTime &&
		fileattr.ftLastWriteTime.dwLowDateTime == aesir_mod_source_fileattr.ftLastWriteTime.dwLowDateTime )
	{
		//printf("AESIR_MOD(Load): No changes detected in \"%S\"\n", aesir_mod_source_filename.c_str() );
		return aesir_mod_shadow_functions;
	}

	auto next_shadow_filename{ aesir_mod_shadow_filename };
    swprintf(const_cast<wchar_t*>(next_shadow_filename.data()) + next_shadow_filename.length() - 5, 6, L"%05.5d", aesir_mod_shadow_version+1 );
	if( !CopyFileW(aesir_mod_source_filename.c_str(), next_shadow_filename.c_str(), FALSE ) )
	{
		printf("AESIR_MOD(Load): Copying \"%S\" to \"%S\" failed with %08.8X\n", aesir_mod_source_filename.c_str(), next_shadow_filename.c_str(), GetLastError() );
		return nullptr;
	}

	auto mod = LoadLibraryW(next_shadow_filename.c_str());
	if( !mod )
	{
		printf("AESIR_MOD(Load): LoadLibraryW(\"%S\", ...) failed with %08.8X\n", next_shadow_filename.c_str(), GetLastError() );
		goto fail;
	}

	typedef void* (*mod_functions)();
	mod_functions mf = reinterpret_cast<mod_functions>(GetProcAddress(mod, "aesir_mod_functions"));
	if( !mf )
	{
		printf("AESIR_MOD(Load): GetProcAddress( <\"%S\">, \"aesir_mod_functions\"), failed with %08.8X\n", next_shadow_filename.c_str(), GetLastError() );
		goto fail;
	}
	
	void* functions = mf();
	if( !functions )
	{
		printf("AESIR_MOD(Load): Calling \"%S\" through the aesir_mod_functions() returned nullptr\n", next_shadow_filename.c_str() );
		goto fail;
	}
	
	printf("AESIR_MOD(Load): Successfully reloaded \"%S\", version=%d\n", next_shadow_filename.c_str(), aesir_mod_shadow_version );
	aesir_mod_source_fileattr = fileattr;
	aesir_mod_shadow_functions = functions;
	aesir_mod_shadow_filename = next_shadow_filename;
	aesir_mod_shadow_version++;
	return functions;

fail:
	if( mod )
		FreeLibrary(mod);
	if( !DeleteFileW(next_shadow_filename.c_str()) )
		aesir_mod_shadow_version++;
	return nullptr;
}
