#pragma once
#include "MemAccess.h"

static const int ModLoaderVer = 1;
static const int GameVer = 1;

struct PatchInfo
{
	void *address;
	const void *data;
	int datasize;
};

struct PatchList
{
	const PatchInfo *Patches;
	int Count;
};

struct PointerInfo
{
	void *address;
	void *data;
};

struct PointerList
{
	const PointerInfo *Pointers;
	int Count;
};

typedef void(__cdecl *ModInitFunc)(const char *path);

struct ModInfo
{
	int LoaderVersion;
	int GameVersion;
};

// define function and variable pointers here
DataPointer(int, ReadFromPack, 0x631C10);
DataPointer(int, ConsoleEnabled, 0x6341A4);

FunctionPointer(int, PrintDebug, (const char *fmt, ...), 0x401140);
FastcallFunctionPointer(void, HashFilename, (char *filename, int *hash), 0x5A0130);
FastcallFunctionPointer(int, LoadFile, (char *filename, void* info), 0x5A0860);
