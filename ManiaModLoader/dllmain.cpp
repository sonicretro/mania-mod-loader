// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <dbghelp.h>
#include <fstream>
#include <memory>
#include <algorithm>
#include <vector>
#include <sstream>
#include "bass.h"
#include "bass_vgmstream.h"
#include "bass_fx.h"
#include "CodeParser.hpp"
#include "IniFile.hpp"
#include "TextConv.hpp"
#include "FileMap.hpp"
#include "FileSystem.h"
#include "Events.h"
#include "Trampoline.h"
#include "ManiaModLoader.h"
#include "Direct3DHook.h"

using std::ifstream;
using std::string;
using std::wstring;
using std::unique_ptr;
using std::vector;
using std::unordered_map;

#define STATUS_OK 200

static bool SteamProtected = false;

/**
* Change write protection of the .trace section.
* @param protect True to protect; false to unprotect.
*/
static void SetRDataWriteProtection(bool protect)
{
	// Reference: https://stackoverflow.com/questions/22588151/how-to-find-data-segment-and-code-segment-range-in-program

	// module handle. (main executable)
	HMODULE hModule = GetModuleHandle(nullptr);

	// Get the PE header.
	const IMAGE_NT_HEADERS *const pNtHdr = ImageNtHeader(hModule);
	// Section headers are located immediately after the PE header.
	const IMAGE_SECTION_HEADER *pSectionHdr = reinterpret_cast<const IMAGE_SECTION_HEADER*>(pNtHdr + 1);

	// Find the .rdata section.
	for (unsigned int i = pNtHdr->FileHeader.NumberOfSections; i > 0; i--, pSectionHdr++)
	{
		if (strncmp(reinterpret_cast<const char*>(pSectionHdr->Name), ".trace", sizeof(pSectionHdr->Name)) != 0)
			continue;

		const intptr_t vaddr = reinterpret_cast<intptr_t>(hModule) + pSectionHdr->VirtualAddress;
		DWORD flOldProtect;
		DWORD flNewProtect = (protect ? PAGE_READONLY : PAGE_WRITECOPY);
		VirtualProtect(reinterpret_cast<void*>(vaddr), pSectionHdr->Misc.VirtualSize, flNewProtect, &flOldProtect);
		return;
	}
}

FileMap fileMap;

static inline void ReplaceToForwardSlash(char* array)
{
	int len = strlen(array);
	for (int i = 0; i < len; ++i)
		if (array[i] == '\\') array[i] = '/';
}

const char* __cdecl GetReplaceablePath(const char* path)
{
	return fileMap.replaceFile(path);
}

void AddReplaceFile(const char* src, const char* dst)
{
	fileMap.addReplaceFile(src, dst);
}

bool _CheckFile(const char* path)
{
	return fileMap.getModIndex(path) != 0;
}

int CheckFile_i(char *buf)
{
	const char *tmp = fileMap.replaceFile(buf);
	bool loadModdedFile = fileMap.getModIndex(buf) != 0;
	if (tmp != buf)
	{
		strncpy(buf, tmp, MAX_PATH);
		if (loadModdedFile)
			return 1;
		else
			ReplaceToForwardSlash(buf);
	}
	return !UseDataPack;
}

int loc_LoadFile = (baseAddress + 0x1C5427);
int loc_LoadDataPack = (baseAddress + 0x1C5417);
__declspec(naked) void CheckFile()
{
	__asm
	{
		lea eax, [ebp + -408h]
		push eax
		call CheckFile_i
		add esp, 4
		test eax, eax
		jnz blah
		jmp loc_LoadDataPack
	blah:
		jmp loc_LoadFile
	}
}

unordered_map<string, unsigned int> musicloops;
Trampoline *musictramp;
int __cdecl PlayMusicFile_Normal(char *name, unsigned int a2, int a3, unsigned int loopstart, int a5)
{
	string namestr = name;
	std::transform(namestr.begin(), namestr.end(), namestr.begin(), tolower);
	auto iter = musicloops.find(namestr);
	if (iter != musicloops.cend())
		loopstart = iter->second;
	auto orig = (decltype(PlayMusicFile_Normal)*)musictramp->Target();
	return orig(name, a2, a3, loopstart, a5);
}


struct struct_0
{
	int anonymous_0;
	int anonymous_1;
	float volume;
	int anonymous_3;
	int anonymous_4;
	int anonymous_5;
	int anonymous_6;
	int hasLoop;
	__int16 anonymous_8;
	_BYTE gap22[1];
	char playStatus;
};

struct MusicInfo
{
	__int16 field_0;
	_BYTE gap2[1];
	char Names[32][16];
	char field_203;
	int LoopStarts[16];
	int field_244;
	int CurrentSong;
	int field_24C;
	int field_250;
	int field_254;
	int field_258;
	int field_25C;
	int field_260;
	int field_264;
};

struct VGMStreamLoopPos
{
	int start;
	int end;
};

void *DecryptBytes_ptr = (void*)(baseAddress + 0x001C5690);
int DecryptBytes(fileinfo *file, void *buffer, int bufferSize)
{
	int result;
	__asm
	{
		push bufferSize
		push buffer
		mov ecx, file
		call DecryptBytes_ptr
		mov result, eax
	}
	return result;
}


void *LoadFile_ptr = (void*)(baseAddress + 0x001C53C0);
__declspec(dllexport) int LoadFile(char *filename, fileinfo *info, void* unknown)
{
	int result;
	__asm
	{
		mov ecx, info
		push unknown
		push filename
		call LoadFile_ptr
		mov result, eax
	}
	return result;
}

__declspec(dllexport) int ReadBytesFromFile(fileinfo* file, void* buffer, int bytes)
{
	int bytesRead = (*(decltype(fread)**)(baseAddress + 0x00243474))(buffer, 1, bytes, file->File);
	if (file->IsEncrypted)
		DecryptBytes(file, buffer, bytes);
	return bytesRead;
}

DataArray(struct_0, stru_26B818, 0x0026B818, 16);
DWORD basschan;
QWORD loopPoint;
char *musicbuf = nullptr;
bool one_up = false;
DWORD lastbasschan = 0;
QWORD lastlooppoint;
char *lastmusicbuf = nullptr;
int oldsong = -1;
char oldstatus = 0;
char* lastsong = new char[MAX_PATH];
bool lastoneup = false;

static void ReleaseLastTrack()
{
	if (lastbasschan != 0)
	{
		BASS_ChannelStop(lastbasschan);
		BASS_StreamFree(lastbasschan);
		if (lastmusicbuf)
		{
			delete[] lastmusicbuf;
			lastmusicbuf = nullptr;
		}
		lastmusicbuf = nullptr;
		lastbasschan = 0;
		one_up = false;
	}
}

static void ReleaseCurrentTrack()
{
	if (basschan != 0)
	{
		BASS_ChannelStop(basschan);
		BASS_StreamFree(basschan);
		if (musicbuf)
		{
			delete[] musicbuf;
			musicbuf = nullptr;
		}
		musicbuf = nullptr;
		basschan = 0;
	}
}

static void ContinueLastTrack()
{
	basschan = lastbasschan;
	lastbasschan = 0;
	loopPoint = lastlooppoint;
	musicbuf = lastmusicbuf;
	lastmusicbuf = nullptr;
	BASS_ChannelPlay(basschan, FALSE);
}


/**
* BASS callback: Current track has ended.
* @param handle
* @param channel
* @param data
* @param user
*/
static void __stdcall onTrackEnd(HSYNC handle, DWORD channel, DWORD data, void *user)
{
	ReleaseCurrentTrack();
	if (one_up)
	{
		ContinueLastTrack();
		one_up = false;
	}
	else if (oldsong != -1)
		oldstatus = stru_26B818[oldsong].playStatus = 0;
}

static void __stdcall LoopTrack(HSYNC handle, DWORD channel, DWORD data, void *user)
{
	BASS_ChannelSetPosition(channel, loopPoint, BASS_POS_BYTE);
}

bool enablevgmstream = false;
bool bluespheretempo = false;
int bluespheretime = -1;

int __cdecl PlayMusicFile_BASS(char *name, unsigned int slot, int a3, unsigned int loopstart, bool createLoopThread)
{
	if (ConsoleEnabled)
		PrintDebug("PlayMusicFile_BASS(\"%s\", %u, %d, %u, %s);\n", name, slot, a3, loopstart, createLoopThread ? "true" : "false");
	if (stru_26B818[slot].playStatus == 3)
		return -1;
	string namestr = name;
	std::transform(namestr.begin(), namestr.end(), namestr.begin(), tolower);
	auto iter = musicloops.find(namestr);
	if (iter != musicloops.cend())
		loopstart = iter->second;
	if (!_stricmp(name, "1up.ogg"))
	{
		if (one_up)
		{
			ReleaseCurrentTrack();
		}
		else
		{
			one_up = true;
			BASS_ChannelPause(basschan);
			lastbasschan = basschan;
			basschan = 0;
			lastlooppoint = loopPoint;
			lastmusicbuf = musicbuf;
			musicbuf = nullptr;
		}
	}
	else
	{
		one_up = false;
		if (lastoneup && !_stricmp(name, lastsong))
		{
			if (lastbasschan)
			{
				ReleaseCurrentTrack();
				ContinueLastTrack();
			}
			lastoneup = false;
			stru_26B818[slot].playStatus = 2;
			return slot;
		}
		else
		{
			ReleaseCurrentTrack();
			ReleaseLastTrack();
		}
	}
	bool useloop = false;
	char buf[MAX_PATH];
	strncpy(buf, "Data/Music/", MAX_PATH);
	strncat(buf, name, MAX_PATH);
	string newname = fileMap.replaceFile(buf);
	string ext = GetExtension(newname);
	if (UseDataPack && newname == buf)
	{
		fileinfo fi;
		memset(&fi, 0, sizeof(fileinfo));
		if (LoadFile(buf, &fi, buf))
		{
			musicbuf = new char[fi.FileSize];
			ReadBytesFromFile(&fi, musicbuf, fi.FileSize);
			if (fi.File)
			{
				(*(decltype(fclose)**)(baseAddress + 0x002434B0))(fi.File);
				fi.File = nullptr;
			}
			useloop = loopstart > 0;
			basschan = BASS_StreamCreateFile(TRUE, musicbuf, 0, fi.FileSize, BASS_STREAM_DECODE | (useloop ? BASS_SAMPLE_LOOP : 0));
		}
	}
	else
	{
		if (ext != "ogg" && ext != "mp3" && ext != "wav")
			basschan = BASS_VGMSTREAM_StreamCreate(newname.c_str(), BASS_STREAM_DECODE | (useloop ? BASS_SAMPLE_LOOP : 0));
		if (basschan == 0)
		{
			useloop = loopstart > 0;
			basschan = BASS_StreamCreateFile(FALSE, newname.c_str(), 0, 0, BASS_STREAM_DECODE | (useloop ? BASS_SAMPLE_LOOP : 0));
		}

		ReplaceFileExtension(newname, ".pos");
		if (Exists(newname))
		{
			FILE* handle = fopen(newname.c_str(), "rb");
			if (handle)
			{
				VGMStreamLoopPos loopfile;
				fread(&loopfile, 1, 8, handle);
				fclose(handle);
				loopstart = loopfile.start;
				useloop = loopstart > 0;
			}
		}
	}
	if (basschan != 0)
	{
		basschan = BASS_FX_TempoCreate(basschan, BASS_FX_FREESOURCE);
		// Stream opened!
		stru_26B818[slot].anonymous_0 = *(int*)(baseAddress + 0x00A5CC88);
		stru_26B818[slot].anonymous_4 = *(int*)(baseAddress + 0x00A5CC8C);
		*(_DWORD *)&stru_26B818[slot].anonymous_8 = 0x3FF00FF;
		stru_26B818[slot].hasLoop = useloop;
		stru_26B818[slot].anonymous_5 = 0;
		stru_26B818[slot].volume = 1;
		stru_26B818[slot].anonymous_1 = 0;
		stru_26B818[slot].anonymous_3 = 0x10000;
		BASS_ChannelSetAttribute(basschan, BASS_ATTRIB_VOL, 0.5f * MusicVolume);
		
		BASS_ChannelPlay(basschan, false);
		if (useloop)
		{
			loopPoint = loopstart * 4;
			BASS_ChannelSetSync(basschan, BASS_SYNC_END | BASS_SYNC_MIXTIME, 0, LoopTrack, nullptr);
		}
		else
			BASS_ChannelSetSync(basschan, BASS_SYNC_END, 0, onTrackEnd, nullptr);

		if (!_stricmp(name, "1up.ogg"))
			lastoneup = true;
		else
			strcpy(lastsong, name);
		stru_26B818[slot].playStatus = 2;
		if (bluespheretempo && !_stricmp(name, "bluespheresspd.ogg"))
			bluespheretime = 0;
		else
			bluespheretime = -1;
		return slot;
	}
	stru_26B818[slot].playStatus = 0;
	return -1;
}

void SpeedUpMusic()
{
	BASS_ChannelSetAttribute(basschan, BASS_ATTRIB_TEMPO, 50.0f);
}

void SlowDownMusic()
{
	BASS_ChannelSetAttribute(basschan, BASS_ATTRIB_TEMPO, 0);
}

VoidFunc(ResumeSound, 0x001BC9C0);
void ResumeSound_BASS()
{
	BASS_ChannelPlay(basschan, false);
	ResumeSound();
}

VoidFunc(PauseSound, 0x001BC960);
void PauseSound_BASS()
{
	BASS_ChannelPause(basschan);
	PauseSound();
}

FunctionPointer(void, ChangeMusicSpeed, (int slot, float volume, float Channelbalance, float PlaybackSpeed), 0x001BC830);
void ChangeMusicSpeed_BASS(int slot, float volume, float Channelbalance, float PlaybackSpeed)
{
	if (ConsoleEnabled)
		PrintDebug("ChangeMusicSpeed_BASS(%u, %.2f, %.2f, %.2f);\n", slot, volume, Channelbalance, PlaybackSpeed);
	ChangeMusicSpeed(slot, volume, Channelbalance, PlaybackSpeed);
	if (slot == 0 && PlaybackSpeed != 0.0f)
	{
		BASS_ChannelSetAttribute(basschan, BASS_ATTRIB_TEMPO, PlaybackSpeed * 100.0f - 100.0f);
		BASS_ChannelSetAttribute(basschan, BASS_ATTRIB_TEMPO_PITCH, round((log(PlaybackSpeed / 1.0f) / 0.05776227f) * 100.0f) / 100.0f);
	}

}

// Code Parser.
static CodeParser codeParser;

float bluespheretempos[]{ 0, 8.6956521739130434782608f, 19.047619047619047619f, 31.578947368421052631f, 47.0588235294117647f };

DataPointer(double, VideoPlayer_Position, 0x00AA75D0);
DataPointer(MusicInfo *, MusicSlots, 0xAC6E08);
static void __cdecl ProcessCodes()
{
	codeParser.processCodeList();
	RaiseEvents(modFrameEvents);
	if (enablevgmstream && MusicSlots != nullptr && basschan != 0)
	{
		int song = MusicSlots->CurrentSong;
		if (song != -1)
		{
			char status = stru_26B818[song].playStatus;
			if (song == oldsong && status != oldstatus)
				switch (status)
				{
				case 0:
					ReleaseCurrentTrack();
					break;
				case 2:
					BASS_ChannelPlay(basschan, false);
					break;
				case 66:
					BASS_ChannelPause(basschan);
					break;
#ifdef _DEBUG
				default:
					PrintDebug("Unknown status code %d\n", status);
					break;
#endif
				}
			oldstatus = status;
			BASS_ChannelSetAttribute(basschan, BASS_ATTRIB_VOL, stru_26B818[song].volume * 0.5f * MusicVolume);
			if (bluespheretime != -1 && bluespheretime < 7200 && status == 2 && ++bluespheretime % 1800 == 0)
				BASS_ChannelSetAttribute(basschan, BASS_ATTRIB_TEMPO, bluespheretempos[bluespheretime / 1800]);
		}
		else
			oldstatus = 0;
		oldsong = song;
	}
	MainGameLoop();
	if (enablevgmstream && MusicSlots != nullptr && SceneID != 133)
	{
		double pos = VideoPlayer_Position;
		if (stru_26B818[MusicSlots->CurrentSong].playStatus == 0 || basschan == 0)
			pos += 1.0/60.0;
		else
			pos = BASS_ChannelBytes2Seconds(basschan, BASS_ChannelGetPosition(basschan, BASS_POS_BYTE));
		if (pos > VideoPlayer_Position)
			VideoPlayer_Position = pos;
	}
	RaiseEvents(modFramePostEvents);
}

static int loc_HookScreenUpdateReturn = baseAddress + 0x0001D7A;
static __declspec(naked) void HookScreenUpdate()
{
	__asm
	{
		mov ecx, [esi + 0x7C]
		or edx, -01
		pushad;
	}
	RaiseEvents(modScreenUpdateEvents);
	__asm
	{
		popad;
		jmp loc_HookScreenUpdateReturn
	}
}

static int loc_HookScreenDrawUpdateReturn = baseAddress + 0x000532B9;
static __declspec(naked) void HookScreenDrawUpdate()
{
	__asm
	{
		pushad;
	}
	RaiseEvents(modScreenDrawUpdateEvents);
	__asm
	{
		popad;
		add esp, 0x04
		mov ebx, eax
		jmp loc_HookScreenDrawUpdateReturn
	}
}

string savepath;
StdcallFunctionPointer(int, TryLoadUserFile, (const char *filename, void *buffer, unsigned int bufSize, int(__cdecl *setStatus)(int)), 0x001ED7F0);
int __stdcall TryLoadUserFile_r(const char *filename, void *buffer, unsigned int bufSize, int (__cdecl *setStatus)(int))
{
	if (ConsoleEnabled)
		PrintDebug("Attempting to load file: %s\n", filename);
	string path = savepath + '\\' + filename;
	if (IsFile(path))
	{
		FILE *f = fopen(path.c_str(), "rb");
		fseek(f, 0, SEEK_END);
		size_t size = ftell(f);
		if (bufSize < size)
			size = bufSize;
		fseek(f, 0, SEEK_SET);
		fread(buffer, 1, size, f);
		fclose(f);
		if (setStatus)
			return setStatus(STATUS_OK);
		return setStatus(500);
	}
	else
	{
		if (ConsoleEnabled)
			PrintDebug("  Does not exist! Loading Steam UserFile!\n");
		return TryLoadUserFile(filename, buffer, bufSize, setStatus);
	}
}

int __stdcall TrySaveUserFile_r(const char *filename, void *buffer, unsigned int bufSize, int (__cdecl *setStatus)(int), unsigned int a5)
{
	if (ConsoleEnabled)
		PrintDebug("Attempting to save file: %s\n", filename);
	if (!IsDirectory(savepath))
		CreateDirectoryA(savepath.c_str(), nullptr);
	string path = savepath + '\\' + filename;
	FILE *f = fopen(path.c_str(), "wb");
	if (!f)
		return setStatus(500);
	fwrite(buffer, 1, bufSize, f);
	fclose(f);
	return setStatus(STATUS_OK);
}

int __stdcall TryDeleteUserFile_r(const char *filename, int(__cdecl *setStatus)(int))
{
	if (ConsoleEnabled)
		PrintDebug("Attempting to delete file: %s\n", filename);
	string path = savepath + '\\' + filename;
	DeleteFileA(path.c_str());
	return setStatus(STATUS_OK);
}


static vector<wstring> split(const wstring &s, wchar_t delim)
{
	vector<wstring> elems;
	std::wstringstream ss(s);
	wstring item;
	while (std::getline(ss, item, delim))
	{
		elems.push_back(item);
	}
	return elems;
}

static const HelperFunctions helperFunctions =
{
	ModLoaderVer,
	&ReadBytesFromFile,
	&AddReplaceFile,
	&GetReplaceablePath,
	&_CheckFile
};

FunctionPointer(int, sub_1CE730, (), 0x1CE730);
int InitMods()
{
	int result = sub_1CE730();
	HookDirect3D();
	if (ConsoleEnabled)
	{
		// Sends output from stdout to the console
		freopen("CONOUT$", "w", stdout);
		// Tells Mania to send the output to stdout instead of the Debugger
		WriteData((bool*)(baseAddress + 0x0026BB1B), true);
	}
	FILE *f_ini = _wfopen(L"mods\\ManiaModLoader.ini", L"r");
	if (!f_ini)
	{
		MessageBox(nullptr, L"mods\\ManiaModLoader.ini could not be read!", L"Mania Mod Loader", MB_ICONWARNING);
		return result;
	}
	unique_ptr<IniFile> ini(new IniFile(f_ini));
	fclose(f_ini);

	SetRDataWriteProtection(false);

	// Get exe's path and filename.
	wchar_t pathbuf[MAX_PATH];
	GetModuleFileName(nullptr, pathbuf, MAX_PATH);
	wstring exepath(pathbuf);
	wstring exefilename;
	string::size_type slash_pos = exepath.find_last_of(L"/\\");
	if (slash_pos != string::npos)
	{
		exefilename = exepath.substr(slash_pos + 1);
		if (slash_pos > 0)
			exepath = exepath.substr(0, slash_pos);
	}

	// Convert the EXE filename to lowercase.
	transform(exefilename.begin(), exefilename.end(), exefilename.begin(), ::towlower);

	// Process the main Mod Loader settings.
	const IniGroup *settings = ini->getGroup("");

	if (ConsoleEnabled)
	{
		PrintDebug("Mania Mod Loader (API version %d), built " __TIMESTAMP__ "\n",
			ModLoaderVer);
#ifdef MODLOADER_GIT_VERSION
#ifdef MODLOADER_GIT_DESCRIBE
		PrintDebug("%s, %s\n", MODLOADER_GIT_VERSION, MODLOADER_GIT_DESCRIBE);
#else /* !MODLOADER_GIT_DESCRIBE */
		PrintDebug("%s\n", MODLOADER_GIT_VERSION);
#endif /* MODLOADER_GIT_DESCRIBE */
#endif /* MODLOADER_GIT_VERSION */
		if (SteamProtected)
			PrintDebug("\nNOTE: Steam Stub DRM Detected!\n\n");
	}

	bool speedshoestempo = false;
	if (!settings->getBool("UseOriginalMusicPlayer") && BASS_Init(-1, 44100, 0, nullptr, nullptr))
	{
		enablevgmstream = true;
		WriteJump((void*)(baseAddress + 0x001BC640), PlayMusicFile_BASS);
		WriteData((char*)(baseAddress + 0x001BCA20), (char)0xC3);
		//WriteData((char*)0x4016A9, (char)0xB8);//
		//WriteData((int*)0x4016AA, 1);//
		//WriteJump((void*)0x4016AE, (void*)0x4016C3);//
		//WriteData((char*)0x401AD9, (char)0xEB);//
		//WriteJump((void*)0x401B7A, (void*)0x401BB4);//
		//WriteJump((void*)0x401C1D, (void*)0x401C39);//
		WriteJump((void*)(baseAddress + 0x001C8705), PauseSound_BASS);
		WriteCall((void*)(baseAddress + 0x001FE743), PauseSound_BASS);
		WriteJump((void*)(baseAddress + 0x001C8727), ResumeSound_BASS);
		WriteCall((void*)(baseAddress + 0x001FE764), ResumeSound_BASS);
		//WriteCall((void*)0x5CAE95, PauseSound);
		//WriteCall((void*)0x5CAEB6, ResumeSound);
		WriteData((char*)(baseAddress + 0x001FD61F), (char)0xEB);
		WriteData((char*)(baseAddress + 0x001BC5AE), (char)0xEB);
		WriteData<8>((void*)(baseAddress + 0x001FD66A), 0x90);
		*(void**)(baseAddress + 0x00A78ED8) = ChangeMusicSpeed_BASS;
		speedshoestempo = settings->getBool("SpeedShoesTempoChange");
		bluespheretempo = settings->getBool("BlueSpheresTempoChange");
	}
	else
		musictramp = new Trampoline((baseAddress + 0x1BC640), (baseAddress + 0x1BC646), PlayMusicFile_Normal);

	vector<std::pair<ModInitFunc, string>> initfuncs;
	vector<std::pair<ModInitFunc, string>> postinitfuncs;
	vector<std::pair<string, string>> errors;
	unordered_map<string, string> filereplaces;

	if (ConsoleEnabled)
		PrintDebug("Loading mods...\n");
	for (unsigned int i = 1; i <= 999; i++)
	{
		char key[8];
		snprintf(key, sizeof(key), "Mod%u", i);
		if (!settings->hasKey(key))
			break;

		const string mod_dirA = "mods\\" + settings->getString(key);
		const wstring mod_dir = L"mods\\" + settings->getWString(key);
		const wstring mod_inifile = mod_dir + L"\\mod.ini";
		FILE *f_mod_ini = _wfopen(mod_inifile.c_str(), L"r");
		if (!f_mod_ini)
		{
			if (ConsoleEnabled)
				PrintDebug("Could not open file mod.ini in \"mods\\%s\".\n", mod_dirA.c_str());
			errors.push_back(std::pair<string, string>(mod_dirA, "mod.ini missing"));
			continue;
		}
		unique_ptr<IniFile> ini_mod(new IniFile(f_mod_ini));
		fclose(f_mod_ini);

		const IniGroup *const modinfo = ini_mod->getGroup("");
		const string mod_nameA = modinfo->getString("Name");
		if (ConsoleEnabled)
			PrintDebug("%u. %s\n", i, mod_nameA.c_str());

		if (ini_mod->hasGroup("IgnoreFiles"))
		{
			const IniGroup *group = ini_mod->getGroup("IgnoreFiles");
			auto data = group->data();
			for (const auto& iter : *data)
			{
				fileMap.addIgnoreFile(iter.first, i);
				if (ConsoleEnabled)
					PrintDebug("Ignored file: %s\n", iter.first.c_str());
			}
		}

		if (ini_mod->hasGroup("ReplaceFiles"))
		{
			const IniGroup *group = ini_mod->getGroup("ReplaceFiles");
			auto data = group->data();
			for (const auto& iter : *data)
			{
				filereplaces[FileMap::normalizePath(iter.first)] =
					FileMap::normalizePath(iter.second);
			}
		}

		// Check for Data replacements.
		const string modSysDirA = mod_dirA + "\\data";
		if (DirectoryExists(modSysDirA))
			fileMap.scanFolder(modSysDirA, i);

		// Check if the mod has a DLL file.
		if (modinfo->hasKeyNonEmpty("DLLFile"))
		{
			for (auto fn : split(modinfo->getWString("DLLFile"), L','))
			{
				// Prepend the mod directory.
				// TODO: SetDllDirectory().
				wstring dll_filename = mod_dir + L'\\' + fn;
				HMODULE module = LoadLibrary(dll_filename.c_str());
				if (module == nullptr)
				{
					DWORD error = GetLastError();
					LPSTR buffer;
					size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buffer, 0, nullptr);

					string message(buffer, size);
					LocalFree(buffer);

					const string dll_filenameA = UTF16toMBS(dll_filename, CP_ACP);
					if (ConsoleEnabled)
						PrintDebug("Failed loading mod DLL \"%s\": %s\n", dll_filenameA.c_str(), message.c_str());
					errors.push_back(std::pair<string, string>(mod_nameA, "DLL error - " + message));
				}
				else
				{
					const ModInfo *info = (const ModInfo *)GetProcAddress(module, "ManiaModInfo");
					if (info)
					{
						if (info->GameVersion == GameVer)
						{
							const HelperFunctions** helperFuncs = (const HelperFunctions**)GetProcAddress(module, "MML_HelperFunctions");
							if (helperFuncs)
								*helperFuncs = &helperFunctions;
							const ModInitFunc init = (const ModInitFunc)GetProcAddress(module, "Init");
							if (init)
								initfuncs.push_back({ init, mod_dirA });
							const ModInitFunc postInit = (const ModInitFunc)GetProcAddress(module, "PostInit");
							if (postInit)
								postinitfuncs.push_back({ postInit, mod_dirA });
							const PatchList *patches = (const PatchList *)GetProcAddress(module, "Patches");
							if (patches)
								for (int j = 0; j < patches->Count; j++)
									WriteData(patches->Patches[j].address, patches->Patches[j].data, patches->Patches[j].datasize);
							const PointerList *jumps = (const PointerList *)GetProcAddress(module, "Jumps");
							if (jumps)
								for (int j = 0; j < jumps->Count; j++)
									WriteJump(jumps->Pointers[j].address, jumps->Pointers[j].data);
							const PointerList *calls = (const PointerList *)GetProcAddress(module, "Calls");
							if (calls)
								for (int j = 0; j < calls->Count; j++)
									WriteCall(calls->Pointers[j].address, calls->Pointers[j].data);
							const PointerList *pointers = (const PointerList *)GetProcAddress(module, "Pointers");
							if (pointers)
								for (int j = 0; j < pointers->Count; j++)
									WriteData((void **)pointers->Pointers[j].address, pointers->Pointers[j].data);
							RegisterEvent(modScreenUpdateEvents, module, "OnScreenUpdate");
							RegisterEvent(modScreenDrawUpdateEvents, module, "OnScreenDrawUpdate");
							RegisterEvent(modFrameEvents, module, "OnFrame");
							RegisterEvent(modFramePostEvents, module, "OnFramePost");
						}
						else
						{
							const string dll_filenameA = UTF16toMBS(dll_filename, CP_ACP);
							if (ConsoleEnabled)
								PrintDebug("File \"%s\" is not built for the current version of the game.\n", dll_filenameA.c_str());
							errors.push_back(std::pair<string, string>(mod_nameA, "Not a valid mod file."));
						}
					}
					else
					{
						const string dll_filenameA = UTF16toMBS(dll_filename, CP_ACP);
						if (ConsoleEnabled)
							PrintDebug("File \"%s\" is not a valid mod file.\n", dll_filenameA.c_str());
						errors.push_back(std::pair<string, string>(mod_nameA, "Not a valid mod file."));
					}
				}
			}
		}
		if (ini_mod->hasGroup("MusicLoops"))
		{
			const IniGroup *const gr = ini_mod->getGroup("MusicLoops");
			for (auto iter = gr->cbegin(); iter != gr->cend(); ++iter)
			{
				string name = iter->first;
				std::transform(name.begin(), name.end(), name.begin(), tolower);
				name.append(".ogg");
				musicloops[name] = std::stoi(iter->second);
			}
		}
		if (enablevgmstream && modinfo->getBool("BlueSpheresTempoChange"))
			bluespheretempo = true;
		if (enablevgmstream && modinfo->getBool("SpeedShoesTempoChange"))
			speedshoestempo = true;
		if (modinfo->getBool("RedirectSaveFile"))
			savepath = mod_dirA + "\\savedata";
	}

	if (speedshoestempo)
	{
		WriteCall((void*)(baseAddress + 0x000A8C08), SpeedUpMusic);
		WriteCall((void*)(baseAddress + 0x000C27E8), SlowDownMusic);
	}

	if (!savepath.empty())
	{
		WriteCall((void*)(baseAddress + 0x1BDFFF), TryLoadUserFile_r);
		WriteCall((void*)(baseAddress + 0x1ECEAD), TryLoadUserFile_r);
		WriteCall((void*)(baseAddress + 0x1BE022), TrySaveUserFile_r);
		WriteCall((void*)(baseAddress + 0x1ECC77), TrySaveUserFile_r);
		WriteCall((void*)(baseAddress + 0x1BE039), TryDeleteUserFile_r);
	}

	if (!errors.empty())
	{
		std::stringstream message;
		message << "The following mods didn't load correctly:" << std::endl;

		for (auto& i : errors)
			message << std::endl << i.first << ": " << i.second;

		MessageBoxA(nullptr, message.str().c_str(), "Mods failed to load", MB_OK | MB_ICONERROR);
	}

	for (const auto& filereplace : filereplaces)
		fileMap.addReplaceFile(filereplace.first, filereplace.second);

	for (unsigned int i = 0; i < initfuncs.size(); i++)
		initfuncs[i].first(initfuncs[i].second.c_str());
	
	if (ConsoleEnabled)
		PrintDebug("Finished loading mods\n");

	// Check for patches.
	ifstream patches_str("mods\\Patches.dat", ifstream::binary);
	if (patches_str.is_open())
	{
		CodeParser patchParser;
		patchParser.setOffset(baseAddress);
		static const char codemagic[6] = { 'c', 'o', 'd', 'e', 'v', '5' };
		char buf[sizeof(codemagic)];
		patches_str.read(buf, sizeof(buf));
		if (!memcmp(buf, codemagic, sizeof(codemagic)))
		{
			int codecount_header;
			patches_str.read((char*)&codecount_header, sizeof(codecount_header));
			if (ConsoleEnabled)
				PrintDebug("Loading %d patches...\n", codecount_header);
			patches_str.seekg(0);
			int codecount = patchParser.readCodes(patches_str);
			if (codecount >= 0)
			{
				if (ConsoleEnabled)
					PrintDebug("Loaded %d patches.\n", codecount);
				patchParser.processCodeList();
			}
			else
			{
				if (ConsoleEnabled)
					PrintDebug("ERROR loading patches: ");
				switch (codecount)
				{
				case -EINVAL:
					if (ConsoleEnabled)
						PrintDebug("Patch file is not in the correct format.\n");
					break;
				default:
					if (ConsoleEnabled)
						PrintDebug("%s\n", strerror(-codecount));
					break;
				}
			}
		}
		else
		{
			if (ConsoleEnabled)
				PrintDebug("Patch file is not in the correct format.\n");
		}
		patches_str.close();
	}

	// Check for codes.
	ifstream codes_str("mods\\Codes.dat", ifstream::binary);
	if (codes_str.is_open())
	{
		codeParser.setOffset(baseAddress);
		static const char codemagic[6] = { 'c', 'o', 'd', 'e', 'v', '5' };
		char buf[sizeof(codemagic)];
		codes_str.read(buf, sizeof(buf));
		if (!memcmp(buf, codemagic, sizeof(codemagic)))
		{
			int codecount_header;
			codes_str.read((char*)&codecount_header, sizeof(codecount_header));
			if (ConsoleEnabled)
				PrintDebug("Loading %d codes...\n", codecount_header);
			codes_str.seekg(0);
			int codecount = codeParser.readCodes(codes_str);
			if (codecount >= 0)
			{
				if (ConsoleEnabled)
					PrintDebug("Loaded %d codes.\n", codecount);
				codeParser.processCodeList();
			}
			else
			{
				if (ConsoleEnabled)
					PrintDebug("ERROR loading codes: ");
				switch (codecount)
				{
				case -EINVAL:
					if (ConsoleEnabled)
						PrintDebug("Code file is not in the correct format.\n");
					break;
				default:
					if (ConsoleEnabled)
						PrintDebug("%s\n", strerror(-codecount));
					break;
				}
			}
		}
		else
		{
			if (ConsoleEnabled)
				PrintDebug("Code file is not in the correct format.\n");
		}
		codes_str.close();
	}

	WriteJump((void*)(baseAddress + 0x1C540E), CheckFile);
	WriteCall((void*)(baseAddress + 0x1FE1BE), ProcessCodes);
	// ScreenUpdate
	WriteData<6>((void*)(baseAddress + 0x0001D74), 0x90);
	WriteJump((void*)(baseAddress + 0x0001D74), HookScreenUpdate);
	// ScreenDrawUpdate
	WriteData<5>((void*)(baseAddress + 0x000532B4), 0x90);
	WriteJump((void*)(baseAddress + 0x000532B4), HookScreenDrawUpdate);

	// Run post init to all mods after the mod loader has finished starting up
	for (auto& postinitfunc : postinitfuncs)
		postinitfunc.first(postinitfunc.second.c_str());

	return result;
}
HMODULE COMCTL32 = LoadLibraryA("comctl32");

// I know, This is horrible, This is only here to allow the modloader to run with the protected exe
void InitProtected()
{
	WriteData((BYTE*)GetProcAddress(COMCTL32, "InitCommonControls"), (BYTE)0x90);
	SteamProtected = true;
	WriteCall((void*)(baseAddress + 0x1FDF69), InitMods);
	return;
}

static const uint8_t verchk_3617885[] = { 0xE8u, 0xC2, 0x07, 0xFDu, 0xFFu, 0x85u, 0xC0u, 0x75, 0x07u, 0xA2u };
static const uint8_t verchk_3617885_Protected[] = { 0x96u, 0x83u, 0xE6u, 0x9Au, 0x25, 0x3C, 0xC7u, 0xE9u };
//E8 72 45 C1 0E E8 4D
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if (memcmp(verchk_3617885, (const char *)(baseAddress + 0x1FDF69), sizeof(verchk_3617885)) != 0)
		{
			if (memcmp(verchk_3617885_Protected, (const char *)(baseAddress + 0x1FDF69), sizeof(verchk_3617885_Protected)) != 0)
				MessageBox(nullptr, L"The mod loader was not designed for this version of the game.\n\nPlease check for an updated version of the loader.\n\nMod functionality will be disabled.", L"Mania Mod Loader for Build 3617885", MB_ICONWARNING);
		}

		if (memcmp(verchk_3617885_Protected, (const char *)(baseAddress + 0x1FDF69), sizeof(verchk_3617885_Protected)) != 0)
			WriteCall((void*)(baseAddress + 0x1FDF69), InitMods);
		else
			WriteJump(GetProcAddress(COMCTL32, "InitCommonControls"), InitProtected);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

