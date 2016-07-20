#include "stdafx.h"
#include <Psapi.h>

__int64** GlobalBasePointer;
__int64 ScriptBasePointer;
__int64 GlobalPattern;
__int64 ScriptPattern;
__int64 *scriptAddress;
int hash = 0x39DA738B;
int displayNameOffset;
HINSTANCE _hinstDLL;

__int64(*GetModelInfo)(int, __int64);
uintptr_t EnableCars::FindPattern(const char *pattern, const char *mask, const char* startAddress, size_t size)
{
	const char* address_end = startAddress + size;
	const auto mask_length = static_cast<size_t>(strlen(mask) - 1);

	for (size_t i = 0; startAddress < address_end; startAddress++)
	{
		if (*startAddress == pattern[i] || mask[i] == '?')
		{
			if (mask[i + 1] == '\0')
			{
				return reinterpret_cast<uintptr_t>(startAddress) - mask_length;
			}

			i++;
		}
		else
		{
			i = 0;
		}
	}

	return 0;
}

uintptr_t EnableCars::FindPattern(const char *pattern, const char *mask)
{
	MODULEINFO module = {};
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &module, sizeof(MODULEINFO));

	return FindPattern(pattern, mask, reinterpret_cast<const char *>(module.lpBaseOfDll), module.SizeOfImage);
}

void EnableCars::FindGlobalAddress() {
	if (GlobalPattern != 0) {
		GlobalBasePointer = (__int64**)(GlobalPattern + *(int*)(GlobalPattern + 3) + 7);
		while (!*GlobalBasePointer)Sleep(100);//Wait for GlobalInitialisation before continuing
		DEBUGMSG("Found global base pointer %llX", (__int64)GlobalBasePointer);
	}
}

void EnableCars::FindScriptAddresses()
{
	if (ScriptPattern != 0) {
		ScriptBasePointer = *(__int64*)(ScriptPattern + *(int*)(ScriptPattern + 3) + 7);
		while ((__int64)ScriptBasePointer == 0)
		{
			Sleep(100);
			ScriptBasePointer = *(__int64*)(ScriptPattern + *(int*)(ScriptPattern + 3) + 7);
		}
		DEBUGMSG("Found script base pointer %llX", ScriptBasePointer);
	}
}


__int64 *EnableCars::getGlobalAddress(int index) {
	return &GlobalBasePointer[index >> 18][index & 0x3FFFF];
}

void EnableCars::FindPatterns()
{
	GlobalPattern = FindPattern("\x4C\x8D\x05\x00\x00\x00\x00\x4D\x8B\x08\x4D\x85\xC9\x74\x11", "xxx????xxxxxxxx");
	ScriptPattern = FindPattern("\x48\x03\x15\x00\x00\x00\x00\x4C\x23\xC2\x49\x8B\x08", "xxx????xxxxxx");
	__int64 CarDisplayNamePattern = FindPattern("\x80\xF9\x05\x75\x08\x48\x05\x00\x00\x00\x00", "xxxxxxx????");
	GetModelInfo = (__int64(*)(int, __int64))(*(int*)(CarDisplayNamePattern - 0x12) + CarDisplayNamePattern - 0x12 + 0x4);
	displayNameOffset = *(int*)(CarDisplayNamePattern + 0x7);

	if (!GlobalPattern)
	{
		Log::Msg("ERROR: Couldnt find Global address");
		Log::Msg("Aborting...");
		FreeLibraryAndExitThread(_hinstDLL, 0);
	}
	if (!ScriptPattern)
	{
		Log::Msg("ERROR: Couldnt find script table address");
		Log::Msg("Aborting...");
		FreeLibraryAndExitThread(_hinstDLL, 0);
	}
}


void EnableCars::Run(HINSTANCE hinstDLL)
{
	_hinstDLL = hinstDLL;
	Log::Init(hinstDLL);
	FindPatterns();
	FindGlobalAddress();
	FindScriptAddresses();
	FindShopController();
	EnableCarsGlobal();
	FreeLibraryAndExitThread(_hinstDLL, 0);
}

void EnableCars::FindShopController()
{
	for (int i = 0; i < 1000; i++)
	{
		if (*(int*)(ScriptBasePointer + (i << 4) + 12) == hash)
		{
			scriptAddress = (__int64*)(ScriptBasePointer + (i << 4));
			if (!*scriptAddress)
			{
				DEBUGMSG("script not loaded, waiting...");
			}
			while (!*scriptAddress)
			{
				Sleep(100);
			}
			DEBUGMSG("Shop Controller script loaded, Address = %llX", *scriptAddress);
			return;
		}
	}
	Log::Msg("ERROR: Couldnt find script address");
	Log::Msg("Aborting...");
	FreeLibraryAndExitThread(_hinstDLL, 0);
}

__int64 EnableCars::GetScriptAddress(__int64* codeBlocksOff, int offset)
{
	return *(codeBlocksOff + (offset >> 14)) + (offset & 0x3FFF);
}

void EnableCars::EnableCarsGlobal()
{
	__int64 scriptBaseAddress = *scriptAddress;
	int CodeLength = *(int*)(scriptBaseAddress + 0x1C);
	int codeBlocks = CodeLength >> 14;
	DEBUGMSG("Script Loaded, CodeLength = %d, CodeBlockCount = %d", CodeLength, codeBlocks);
	__int64* CodeBlockOffset = *(__int64**)(scriptBaseAddress + 0x10);
	for (int i = 0; i < codeBlocks; i++)
	{
		//__int64 sigAddress = FindPattern("\x28\x26\xCE\x6B\x86\x39\x03\x55\x00\x00\x28\xF0\x91\xDE\xBC", "xxxxxxxx??xxxxx", (const char*)(*(CodeBlockOffset + i)), (size_t)0x4000);
		__int64 sigAddress = FindPattern("\x28\x26\xCE\x6B\x86\x39\x03", "xxxxxxx", (const char*)(*(CodeBlockOffset + i)), 0x4000);
		if (!sigAddress)
		{
			continue;
		}
		DEBUGMSG("Pattern found in codepage %d at memory address %llX", i, sigAddress);
		int RealCodeOff = (int)(sigAddress - *(CodeBlockOffset + i) + (i << 14));
		for (int j = 0; j < 400; j++)
		{
			if (*(int*)(GetScriptAddress(CodeBlockOffset, RealCodeOff - j)) == 0x0008012D)
			{
				int funcOff = *(int*)GetScriptAddress(CodeBlockOffset, RealCodeOff - j + 6) & 0xFFFFFF;
				DEBUGMSG("found function codepage address at %x", funcOff);
				for (int k = 0x5; k < 0x40; k++)
				{
					if ((*(int*)GetScriptAddress(CodeBlockOffset, funcOff + k) & 0xFFFFFF) == 0x01002E)
					{
						for (k = k + 1; k < 30; k++)
						{
							if (*(unsigned char*)GetScriptAddress(CodeBlockOffset, funcOff + k) == 0x5F)
							{
								int globalindex = *(int*)GetScriptAddress(CodeBlockOffset, funcOff + k + 1) & 0xFFFFFF;
								DEBUGMSG("Found Global Variable %d, address = %llX", globalindex, (__int64)getGlobalAddress(globalindex));
								Log::Msg("Setting Global Variable %d to true", globalindex);
								*getGlobalAddress(globalindex) = 1;
								Log::Msg("MP Cars enabled");
								FindSwitch(CodeBlockOffset, RealCodeOff - j);//not essential, just for printing affected cars
								return;
							}
						}
						break;
					}
				}
				break;
			}
		}
		break;
	}
	Log::Msg("Global Variable not found, check game version >= 1.0.678.1");
}

void EnableCars::FindSwitch(__int64* codeBlockOffset, int funcCallOffset)
{
	for (int i = 14; i<400;i++)
	{
		if (*(unsigned char*)GetScriptAddress(codeBlockOffset, funcCallOffset + i) == 0x62)
		{
			FindAffectedCars(codeBlockOffset, funcCallOffset + i);
			return;
		}
	}
	Log::Msg("Couldnt find affected cars");
}

void EnableCars::FindAffectedCars(__int64* codeBlockOffset, int scriptSwitchOffset)
{
	__int64 curAddress = (__int64)GetScriptAddress(codeBlockOffset, scriptSwitchOffset + 2);
	int startOff = scriptSwitchOffset + 2;
	int cases = *(unsigned char*)(curAddress - 1);
	for (int i = 0; i<cases;i++)
	{
		curAddress += 6;
		startOff += 6;
		int jumpoff = *(short*)(curAddress - 2);
		__int64 caseOff = GetScriptAddress(codeBlockOffset, startOff + jumpoff);
		unsigned char opCode = *(unsigned char*)caseOff;
		int hash;
		if (opCode == 0x28)//push int
		{
			hash = *(int*)(caseOff + 1);
		}else if(opCode == 0x61)//push int 24
		{
			hash = *(int*)(caseOff + 1) & 0xFFFFFF;
		}else
		{
			Log::Msg("Error with car hash %X", *(int*)(caseOff + 1));// in reality this should never be executes
			continue;
		}
		Log::Msg("Found affected hash: 0x%X - DisplayName = %s", hash, GetDisplayName(hash));
	}
}
#ifdef _DEBUG
bool one = false;
#endif
char* EnableCars::GetDisplayName(int modelHash)
{
	int data = 0xFFFF;
	__int64 addr = GetModelInfo(modelHash, (__int64)&data);
#ifdef _DEBUG
	if (!one)
	{
		one = true;
		DEBUGMSG("DEBUG Address = %llX", addr);
	}
#endif
	if (addr && (*(unsigned char*)(addr + 157) & 0x1F) == 5)
	{
		return (char*)(addr + displayNameOffset);
	}
	else
	{
		return "CARNOTFOUND";
	}
}


