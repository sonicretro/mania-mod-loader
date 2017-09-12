// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <dbghelp.h>
#include <fstream>
#include <memory>
#include <algorithm>
#include <vector>
#include <sstream>
#include "CodeParser.hpp"
#include "IniFile.hpp"
#include "TextConv.hpp"
#include "FileMap.hpp"
#include "FileSystem.h"
#include "Events.h"
#include "ManiaModLoader.h"

using std::ifstream;
using std::string;
using std::wstring;
using std::unique_ptr;
using std::vector;

/**
* Change write protection of the .xtext section.
* @param protect True to protect; false to unprotect.
*/
static void SetXTextWriteProtection(bool protect)
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
		if (strncmp(reinterpret_cast<const char*>(pSectionHdr->Name), ".xtext", sizeof(pSectionHdr->Name)) != 0)
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

int loc_5A08DB = 0x5A08DB;
int loc_5A097B = 0x5A097B;
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
		jmp loc_5A097B
	blah:
		jmp loc_5A08DB
	}
}

// Code Parser.
static CodeParser codeParser;

static void __cdecl ProcessCodes()
{
	codeParser.processCodeList();
	RaiseEvents(modFrameEvents);
	MainGameLoop();
}

VoidFunc(sub_5BD1C0, 0x5BD1C0);
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

	SetXTextWriteProtection(false);

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
			// Prepend the mod directory.
			// TODO: SetDllDirectory().
			wstring dll_filename = mod_dir + L'\\' + modinfo->getWString("DLLFile");
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

	WriteJump((void*)0x5A08CE, CheckFile);
	WriteCall((void*)0x5CAAFF, ProcessCodes);

	sub_5BD1C0();
}

static const char verchk[] = { 0xE8u, 0x34, 0x2C, 0xFFu, 0xFFu, 0xE8u, 0xFFu, 0x6E, 0xFDu, 0xFFu };
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if (memcmp(verchk, (const char *)0x5CA587, sizeof(verchk)) != 0)
			MessageBox(nullptr, L"The mod loader was not designed for this version of the game.\n\nPlease check for an updated version of the loader.\n\nMod functionality will be disabled.", L"Mania Mod Loader", MB_ICONWARNING);
		else
			WriteCall((void*)0x5CA587, InitMods);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

