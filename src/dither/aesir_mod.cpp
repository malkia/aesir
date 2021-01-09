#pragma optimize("", off)

#include "aesir_mod.h"

#include <windows.h>

#include <string>

static std::wstring aesir_win32_module_filename( HMODULE module )
{
	std::wstring name;
	DWORD nameLength = 4;
	for( ;; ) {
		name.resize(nameLength + 1);
		auto returned = GetModuleFileNameW(module, const_cast<wchar_t*>(name.data()), nameLength);
		if( returned < nameLength )
		{
			name.resize(returned);
			break;
		}
		nameLength *= 2;
	}
	return name;
}

static std::wstring aesir_win32_resolve_filename( const std::wstring& fileName )
{
	std::wstring name;
	auto h = CreateFileW(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if( h == INVALID_HANDLE_VALUE )
		return name;
	DWORD nameLength = 4;
	for( ;; ) {
		name.resize(nameLength + 1);
		auto returned = GetFinalPathNameByHandleW(h, const_cast<wchar_t*>(name.data()), nameLength, 0 );
		if( returned < nameLength )
		{
			name.resize(returned);
			break;
		}
		nameLength = returned;
	}
	CloseHandle(h);
	return name;
}

static WIN32_FILE_ATTRIBUTE_DATA aesir_mod_source_fileattr;
static std::wstring              aesir_mod_source_filename;
static void*                     aesir_mod_shadow_functions; // cached functions
static std::wstring              aesir_mod_shadow_filename;
static int                       aesir_mod_shadow_version;
static const std::wstring		 aesir_mod_tmp_pattern(L".tmp.00000");

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
        return false;

	aesir_mod_source_filename = moduleName;
    aesir_mod_shadow_filename = pluginName + aesir_mod_tmp_pattern;
	return true;
}

void* aesir_mod_load()
{
    if( !aesir_mod_init() )
		return nullptr;
	
    WIN32_FILE_ATTRIBUTE_DATA fileattr;
    auto b = GetFileAttributesExW(aesir_mod_source_filename.c_str(), GetFileExInfoStandard, &fileattr);
    if( !b )
        return nullptr;

	if( fileattr.nFileSizeHigh == aesir_mod_source_fileattr.nFileSizeHigh &&
		fileattr.nFileSizeLow == aesir_mod_source_fileattr.nFileSizeLow &&
		fileattr.ftCreationTime.dwHighDateTime == aesir_mod_source_fileattr.ftCreationTime.dwHighDateTime &&
		fileattr.ftCreationTime.dwLowDateTime == aesir_mod_source_fileattr.ftCreationTime.dwLowDateTime &&
		fileattr.ftLastWriteTime.dwHighDateTime == aesir_mod_source_fileattr.ftLastWriteTime.dwHighDateTime &&
		fileattr.ftLastWriteTime.dwLowDateTime == aesir_mod_source_fileattr.ftLastWriteTime.dwLowDateTime )
	{
		return aesir_mod_shadow_functions;
	}

	auto next_shadow_filename{ aesir_mod_shadow_filename };
    swprintf(const_cast<wchar_t*>(next_shadow_filename.data()) + next_shadow_filename.length() - 5, 6, L"%05.5d", aesir_mod_shadow_version+1 );
	//next_shadow_filename = L"D:\\p\\aesir\\bin\\Debug\\tmp.v" + std::to_wstring(aesir_mod_shadow_version);
	if( !CopyFileW(aesir_mod_source_filename.c_str(), next_shadow_filename.c_str(), FALSE ) )
	{
		auto hr = GetLastError();
		return nullptr;
	}

	auto mod = LoadLibraryW(next_shadow_filename.c_str());
	if( !mod )
		goto fail;

	typedef void* (*mod_functions)();
	mod_functions mf = reinterpret_cast<mod_functions>(GetProcAddress(mod, "aesir_mod_functions"));
	if( !mf )
		goto fail;
	
	void* functions = mf();
	if( !functions )
		goto fail;
	
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
