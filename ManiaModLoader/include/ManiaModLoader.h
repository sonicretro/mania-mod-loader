#pragma once
#include <cstdio>
#include "MemAccess.h"

static const int ModLoaderVer = 1;
static const int GameVer = 5;

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
	Characters_None		= 0b00000,
	Characters_Sonic	= 0b00001,
	Characters_Tails	= 0b00010,
	Characters_Knuckles = 0b00100,
	Characters_Mighty	= 0b01000,
	Characters_Ray		= 0b10000
};

struct fileinfo
{
	int FileSize;
	_DWORD Unknown04;
	FILE* File;
	int Unknown0C;
	int Unknown10;
	BYTE* FileAddress;
	bool Unknown18;
	bool IsEncrypted;
	_BYTE decryptionkey[32];
	_BYTE gap3A[0x2000];
	/*_DWORD fileoff;
	_DWORD size;
	_DWORD dword4010;
	_DWORD dword4014;
	_DWORD dword4018;
	_DWORD dword401C;
	_DWORD encrypted;
	_DWORD dword4024;
	_BYTE decryptionkey[32];
	_WORD word4048;
	_BYTE byte404A;*/
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

// TODO: Player structure has changed a little, This needs updating
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
DataPointer(bool, UseDataPack, 0x002FC865);
DataPointer(bool, ConsoleEnabled, 0x002FC867);

DataPointer(float, MusicVolume, 0x00A53078);
DataPointer(PlayerData, Player1Data, 0x00469A10);
DataPointer(PlayerData, Player2Data, 0x00469E68);
DataPointer(unsigned char, SceneID, 0x00A535C4); 
DataPointer(int, TimerCentiframes, 0x00A535B8);
DataPointer(int, DebugEnabled, 0x00A535D4);
DataPointer(int, UpdateTimer, 0x00A535DC);
DataPointer(char, GameMode, 0x00A535E2);
DataPointer(char, TimerCentiseconds, 0x00A535E4);
DataPointer(char, TimerSeconds, 0x00A535E5);
DataPointer(char, TimerMinutes, 0x00A535E6);

FunctionPointer(int, PlaySoundFX, (_WORD wavIndex, int a2, _BYTE playLength), 0x001BC390);
FunctionPointer(_WORD, GetSoundFXID, (const char *name), 0x001BC2F0);

//FunctionPointer(int, PrintDebug, (const char* fmt, ...), 0x1280);
#define PrintDebug printf
//
VoidFunc(InitPlayer, 0x000C33F0);
FastcallFunctionPointer(int, Player_CheckGoSuper, (PlayerData *player, int emeraldflags), 0x4C8280);
VoidFunc(Sonic_JumpAbilities, 0x000C8630);
VoidFunc(Tails_JumpAbilities, 0x000C8990);
VoidFunc(Knuckles_JumpAbilities, 0x000CBA70);
VoidFunc(Mighty_JumpAbilities, 0x000C8B70);
VoidFunc(Ray_JumpAbilities, 0x000C8DF0);
VoidFunc(Sonic_CheckDoPeelOut, 0x000C8FF0);
VoidFunc(ERZSuperSonic_JumpAbilities, 0x000C2340);
FastcallFunctionPointer(void, HashFilename, (char *filename, int *hash), 0x1CB620);
VoidFunc(MainGameLoop, 0x001C7CE0);
VoidFunc(IncrementTimer, 0x001F2600);
