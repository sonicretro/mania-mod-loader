#pragma once
#include <cstdio>
#include "MemAccess.h"

static const int ModLoaderVer = 1;
static const int GameVer = 2;

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

typedef uint32_t _DWORD;
typedef uint16_t _WORD;
typedef uint8_t _BYTE;

enum ShieldTypes
{
	ShieldTypes_None,
	ShieldTypes_Regular,
	ShieldTypes_Bubble,
	ShieldTypes_Flame,
	ShieldTypes_Lightning
};

enum Characters
{
	Characters_None,
	Characters_Sonic,
	Characters_Tails,
	Characters_AlsoSonic,
	Characters_Knuckles
};

struct fileinfo
{
	FILE *file;
	_DWORD dword4;
	_BYTE gap8[16384];
	_DWORD fileoff;
	_DWORD size;
	_DWORD dword4010;
	_DWORD dword4014;
	_DWORD dword4018;
	_DWORD dword401C;
	_DWORD encrypted;
	_DWORD dword4024;
	_BYTE decryptionkey[32];
	_WORD word4048;
	_BYTE byte404A;
};

struct fileheader
{
	int filenamehash[4];
	int offset;
	int size;
	int encrypted;
	char index;
	char anonymous_8[3];
};

struct PlayerData
{
	char field_0[2];
	__int16 XPos;
	char field_4[2];
	__int16 YPos;
	_DWORD dword8;
	_DWORD dwordC;
	int XSpeed;
	int YSpeed;
	char field_18[40];
	_DWORD dword40;
	_BYTE gap44[8];
	_BYTE byte4C;
	char field_4D;
	_BYTE byte4E;
	_BYTE byte4F;
	_BYTE gap50[2];
	_BYTE byte52;
	char field_53;
	_BYTE byte54;
	char field_55[3];
	_DWORD dword58;
	_BYTE gap5C[60];
	_DWORD dword98;
	_DWORD dword9C;
	_DWORD dwordA0;
	char field_A4[12];
	_WORD wordB0;
	_WORD wordB2;
	_BYTE gapB4[2];
	_WORD wordB6;
	char field_B8[8];
	_DWORD Character;
	_DWORD RingCount;
	_DWORD dwordC8;
	_DWORD ShieldAbility;
	_DWORD LifeCount;
	_DWORD dwordD4;
	_DWORD dwordD8;
	_DWORD CombineRing;
	char field_E0[68];
	int SuperMode;
	char field_128[12];
	_DWORD dword134;
	_BYTE gap138[4];
	_DWORD dword13C;
	_BYTE gap140[60];
	_DWORD dword17C;
	_DWORD dword180;
	_DWORD dword184;
	_DWORD dword188;
	_DWORD dword18C;
	_DWORD dword190;
	_BYTE gap194[8];
	_DWORD dword19C;
	_DWORD dword1A0;
	char field_1A4[32];
	int JumpAbilityFlag;
	void(__cdecl *JumpAbilities)();
	void(__cdecl *PeelOutThing)();
};

// define function and variable pointers here
DataPointer(int, ReadFromPack, 0x631C10);
DataPointer(int, ConsoleEnabled, 0x6341A4);
DataPointer(PlayerData, Player1Data, 0xA4D6C0);
DataPointer(PlayerData, Player2Data, 0xA4DB18);
DataPointer(int, TimerCentiframes, 0xCD06EC);
DataPointer(int, DebugEnabled, 0xCD0708);
DataPointer(int, UpdateTimer, 0xCD0710);
DataPointer(char, GameMode, 0xCD0716);
DataPointer(char, TimerCentiseconds, 0xCD0717);
DataPointer(char, TimerSeconds, 0xCD0718);
DataPointer(char, TimerMinutes, 0xCD0719);

FunctionPointer(int, PrintDebug, (const char *fmt, ...), 0x401140);
VoidFunc(InitPlayer, 0x47F650);
FastcallFunctionPointer(int, Player_CheckGoSuper, (PlayerData *a1, int emeraldflags), 0x483360);
VoidFunc(Sonic_JumpAbilities, 0x483690);
VoidFunc(Tails_JumpAbilities, 0x4839D0);
VoidFunc(Knuckles_JumpAbilities, 0x483AA0);
VoidFunc(Sonic_CheckDoPeelOut, 0x483B90);
FastcallFunctionPointer(void, HashFilename, (char *filename, int *hash), 0x5A0130);
FastcallFunctionPointer(int, LoadFile, (char *filename, fileinfo *info), 0x5A0860);
VoidFunc(MainGameLoop, 0x5A1EC0);
VoidFunc(IncrementTimer, 0x5A53F0);
VoidFunc(ERZSuperSonic_JumpAbilities, 0x1989290);
