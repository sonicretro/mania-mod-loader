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

using std::ifstream;
using std::string;
using std::wstring;
using std::unique_ptr;
using std::vector;
using std::unordered_map;

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

int CheckFile_i(char *buf)
{
	if (fileMap.getModIndex(buf) != 0)
	{
		const char *tmp = fileMap.replaceFile(buf);
		strncpy(buf, tmp, MAX_PATH);
		return 1;
	}
	return !ReadFromPack;
}

int loc_5A07DB = 0x5A07DB;
int loc_5A087B = 0x5A087B;
__declspec(naked) void CheckFile()
{
	__asm
	{
		lea eax, [esp + 20h]
		push eax
		call CheckFile_i
		add esp, 4
		test eax, eax
		jnz blah
		jmp loc_5A087B
	blah:
		jmp loc_5A07DB
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

bool enablevgmstream;
DWORD basschan;
char *musicbuf = nullptr;

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

void *ReadFileData_ptr = (void*)0x5A0AA0;
inline int ReadFileData(void *buffer, fileinfo *a2, unsigned int _size)
{
	int result;
	__asm
	{
		push [_size]
		mov ecx, [a2]
		mov edx, [buffer]
		call ReadFileData_ptr
		add esp, 4
		mov result, eax
	}
	return result;
}

DataArray(struct_0, stru_D79CA0, 0xD79CA0, 16);
int oldsong = -1;
char oldstatus = 0;
/**
* BASS callback: Current track has ended.
* @param handle
* @param channel
* @param data
* @param user
*/
static void __stdcall onTrackEnd(HSYNC handle, DWORD channel, DWORD data, void *user)
{
	BASS_ChannelStop(channel);
	BASS_StreamFree(channel);
	if (oldsong != -1)
		oldstatus = stru_D79CA0[oldsong].playStatus = 0;
}

QWORD loopPoint;
static void __stdcall LoopTrack(HSYNC handle, DWORD channel, DWORD data, void *user)
{
	BASS_ChannelSetPosition(channel, loopPoint, BASS_POS_BYTE);
}

bool bluespheretempo = false;
int bluespheretime = -1;

int __cdecl PlayMusicFile_BASS(char *name, unsigned int a2, int a3, unsigned int loopstart, int a5)
{
	if (stru_D79CA0[a2].playStatus == 3)
		return -1;
	string namestr = name;
	std::transform(namestr.begin(), namestr.end(), namestr.begin(), tolower);
	auto iter = musicloops.find(namestr);
	if (iter != musicloops.cend())
		loopstart = iter->second;
	if (basschan != 0)
	{
		BASS_ChannelStop(basschan);
		BASS_StreamFree(basschan);
		basschan = 0;
	}
	if (musicbuf)
	{
		delete[] musicbuf;
		musicbuf = nullptr;
	}
	bool useloop = false;
	char buf[MAX_PATH];
	strncpy(buf, "Data/Music/", MAX_PATH);
	strncat(buf, name, MAX_PATH);
	string newname = fileMap.replaceFile(buf);
	string ext = GetExtension(newname);
	if (newname == buf)
	{
		fileinfo fi;
		if (LoadFile(buf, &fi) == 1)
		{
			musicbuf = new char[fi.size];
			ReadFileData(musicbuf, &fi, fi.size);
			if (fi.file)
			{
				((decltype(fclose)*)0x37FB164)(fi.file);
				fi.file = nullptr;
			}
			useloop = loopstart > 0;
			basschan = BASS_StreamCreateFile(TRUE, musicbuf, 0, fi.size, BASS_STREAM_DECODE | (useloop ? BASS_SAMPLE_LOOP : 0));
		}
	}
	else
	{
		if (ext != "ogg" && ext != "mp3")
			basschan = BASS_VGMSTREAM_StreamCreate(newname.c_str(), BASS_STREAM_DECODE);
		if (basschan == 0)
		{
			useloop = loopstart > 0;
			basschan = BASS_StreamCreateFile(FALSE, newname.c_str(), 0, 0, BASS_STREAM_DECODE | (useloop ? BASS_SAMPLE_LOOP : 0));
		}
	}
	if (basschan != 0)
	{
		basschan = BASS_FX_TempoCreate(basschan, BASS_FX_FREESOURCE);
		// Stream opened!
		stru_D79CA0[a2].anonymous_0 = *(int*)0xD7BEF0;
		stru_D79CA0[a2].anonymous_4 = *(int*)0xD7BEF4;
		*(_DWORD *)&stru_D79CA0[a2].anonymous_8 = 0x3FF00FF;
		stru_D79CA0[a2].hasLoop = useloop;
		stru_D79CA0[a2].anonymous_5 = 0;
		stru_D79CA0[a2].volume = 1;
		stru_D79CA0[a2].anonymous_1 = 0;
		stru_D79CA0[a2].anonymous_3 = 0x10000;
		BASS_ChannelSetAttribute(basschan, BASS_ATTRIB_VOL, MusicVolume * 0.5f);
		BASS_ChannelPlay(basschan, false);
		if (useloop)
		{
			loopPoint = loopstart * 4;
			BASS_ChannelSetSync(basschan, BASS_SYNC_END | BASS_SYNC_MIXTIME, 0, LoopTrack, nullptr);
		}
		else
			BASS_ChannelSetSync(basschan, BASS_SYNC_END, 0, onTrackEnd, nullptr);
		stru_D79CA0[a2].playStatus = 2;
		if (bluespheretempo && !_stricmp(name, "bluespheresspd.ogg"))
			bluespheretime = 0;
		else
			bluespheretime = -1;
		return a2;
	}
	stru_D79CA0[a2].playStatus = 0;
	return -1;
}

void SpeedUpMusic()
{
	BASS_ChannelSetAttribute(basschan, BASS_ATTRIB_TEMPO, 0.5f);
}

void SlowDownMusic()
{
	BASS_ChannelSetAttribute(basschan, BASS_ATTRIB_TEMPO, 0);
}

VoidFunc(sub_599640, 0x599640);
void ResumeSound()
{
	BASS_ChannelPlay(basschan, false);
	sub_599640();
}

VoidFunc(sub_5995E0, 0x5995E0);
void PauseSound()
{
	BASS_ChannelPause(basschan);
	sub_5995E0();
}

// Code Parser.
static CodeParser codeParser;

DataPointer(MusicInfo *, MusicSlots, 0xD83664);
static void __cdecl ProcessCodes()
{
	codeParser.processCodeList();
	RaiseEvents(modFrameEvents);
	if (MusicSlots != nullptr && basschan != 0)
	{
		int song = MusicSlots->CurrentSong;
		if (song != -1)
		{
			char status = stru_D79CA0[song].playStatus;
			if (song == oldsong && status != oldstatus)
				switch (status)
				{
				case 0:
					BASS_ChannelStop(basschan);
					BASS_StreamFree(basschan);
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
			BASS_ChannelSetAttribute(basschan, BASS_ATTRIB_VOL, stru_D79CA0[song].volume * 0.5f * MusicVolume);
			if (bluespheretime != -1 && bluespheretime < 7200 && status == 2 && ++bluespheretime % 1800 == 0)
			{
				BASS_ChannelSetAttribute(basschan, BASS_ATTRIB_TEMPO, (100 / (100 - (8 * bluespheretime / 1800.0f))) - 1);
				PrintDebug("Speeding up\n");
			}
		}
		else
			oldstatus = 0;
		oldsong = song;
	}
	MainGameLoop();
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

VoidFunc(sub_5BD0E0, 0x5BD0E0);
void InitMods()
{
	FILE *f_ini = _wfopen(L"mods\\ManiaModLoader.ini", L"r");
	if (!f_ini)
	{
		MessageBox(nullptr, L"mods\\ManiaModLoader.ini could not be read!", L"Mania Mod Loader", MB_ICONWARNING);
		return;
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
	}

	bool speedshoestempo = false;
	if (enablevgmstream = (bool)BASS_Init(-1, 44100, 0, nullptr, nullptr))
	{
		WriteJump((void*)0x5992C0, PlayMusicFile_BASS);
		WriteData((char*)0x5996A0, (char)0xC3);
		WriteData((char*)0x4016A7, (char)0xEB);
		WriteData((char*)0x401AD9, (char)0xEB);
		WriteCall((void*)0x5CADA8, PauseSound);
		WriteCall((void*)0x5CADEB, ResumeSound);
		WriteCall((void*)0x5CAE95, PauseSound);
		WriteCall((void*)0x5CAEB6, ResumeSound);
		WriteData((char*)0x5C95DF, (char)0xEB);
		speedshoestempo = settings->getBool("SpeedShoesTempoChange");
		bluespheretempo = settings->getBool("BlueSpheresTempoChange");
	}
	else
		musictramp = new Trampoline(0x5992C0, 0x5992C6, PlayMusicFile_Normal);

	vector<std::pair<ModInitFunc, string>> initfuncs;
	vector<std::pair<string, string>> errors;

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
							const ModInitFunc init = (const ModInitFunc)GetProcAddress(module, "Init");
							if (init)
								initfuncs.push_back({ init, mod_dirA });
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
							RegisterEvent(modFrameEvents, module, "OnFrame");
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
		if (modinfo->getBool("BlueSpheresTempoChange"))
			bluespheretempo = true;
		if (enablevgmstream && modinfo->getBool("SpeedShoesTempoChange"))
			speedshoestempo = true;
	}

	if (speedshoestempo)
	{
		WriteCall((void*)0x43DCE6, SpeedUpMusic);
		WriteCall((void*)0x47EA16, SlowDownMusic);
	}

	if (!errors.empty())
	{
		std::stringstream message;
		message << "The following mods didn't load correctly:" << std::endl;

		for (auto& i : errors)
			message << std::endl << i.first << ": " << i.second;

		MessageBoxA(nullptr, message.str().c_str(), "Mods failed to load", MB_OK | MB_ICONERROR);
	}

	for (unsigned int i = 0; i < initfuncs.size(); i++)
		initfuncs[i].first(initfuncs[i].second.c_str());

	if (ConsoleEnabled)
		PrintDebug("Finished loading mods\n");

	// Check for patches.
	ifstream patches_str("mods\\Patches.dat", ifstream::binary);
	if (patches_str.is_open())
	{
		CodeParser patchParser;
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

	WriteJump((void*)0x5A07CE, CheckFile);
	WriteCall((void*)0x5CAA0F, ProcessCodes);

	sub_5BD0E0();
}

static const char verchk[] = { 0xE8u, 0x44, 0x2C, 0xFFu, 0xFFu, 0xE8u, 0xEFu, 0x6E, 0xFDu, 0xFFu };
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if (memcmp(verchk, (const char *)0x5CA497, sizeof(verchk)) != 0)
			MessageBox(nullptr, L"The mod loader was not designed for this version of the game.\n\nPlease check for an updated version of the loader.\n\nMod functionality will be disabled.", L"Mania Mod Loader", MB_ICONWARNING);
		else
			WriteCall((void*)0x5CA497, InitMods);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

