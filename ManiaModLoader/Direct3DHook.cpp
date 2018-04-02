// maniamodloader.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <cstdio>
#include "Direct3DHook.h"

void *Direct3DCreate9Ptr;

void HookDirect3D()
{
	wchar_t windir[MAX_PATH];
	GetSystemDirectory(windir, MAX_PATH);
	wchar_t d3dpath[MAX_PATH];
	_snwprintf(d3dpath, MAX_PATH, L"%s\\d3d9.dll", windir);
	const HMODULE hmod = LoadLibrary(d3dpath);
	Direct3DCreate9Ptr = GetProcAddress(hmod, "Direct3DCreate9");
}

extern "C"
{
	__declspec(dllexport) __declspec(naked) void Direct3DCreate9()
	{
		__asm
		{
			jmp Direct3DCreate9Ptr
		}
	}
}
