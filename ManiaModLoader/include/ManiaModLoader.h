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

typedef void(__cdecl *ModEvent)();

struct ModInfo
{
	int LoaderVersion;
	int GameVersion;
};

// define function and variable pointers here
DataPointer(int, ReadFromPack, 0x631C10);
DataPointer(int, ConsoleEnabled, 0x6341A4);
DataPointer(int, RingCount, 0xA4D644);
DataPointer(int, LifeCount, 0xA4D650);
DataPointer(int, CombineRing, 0xA4D65C);
DataPointer(int, TimerCentiframes, 0xCD05AC);
DataPointer(int, DebugEnabled, 0xCD05C8);
DataPointer(int, UpdateTimer, 0xCD05D0);
DataPointer(char, GameMode, 0xCD05D6);
DataPointer(char, TimerCentiseconds, 0xCD05D7);
DataPointer(char, TimerSeconds, 0xCD05D8);
DataPointer(char, TimerMinutes, 0xCD05D9);

FunctionPointer(int, PrintDebug, (const char *fmt, ...), 0x401140);
FastcallFunctionPointer(void, HashFilename, (char *filename, int *hash), 0x5A0130);
FastcallFunctionPointer(int, LoadFile, (char *filename, void* info), 0x5A0860);
VoidFunc(MainGameLoop, 0x5A1EC0);
VoidFunc(IncrementTimer, 0x5A53F0);
