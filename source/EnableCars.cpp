#include "stdafx.h"
#include <Psapi.h>
#include <vector>
struct HashPair
{
	int modelHash;
	int DispHash;
};
__int64** GlobalBasePointer;
__int64 ScriptBasePointer;
__int64 GlobalPattern;
__int64 ScriptPattern;
__int64 *shopControllerAddress;
__int64 *cheatControllerAddress;
int displayNameOffset;
HINSTANCE _hinstDLL;
std::vector<HashPair> CarHashes;
__int64(*GetModelInfo)(int, __int64);
/*dont even bother trying to understand this
What it evaluates to is
void SpawnCarCheck(Hash modelHash, Hash displayHash)
{
	Vehicle Temp;
	if (GAMEPLAY::_HAS_CHEAT_STRING_JUST_BEEN_ENTERED(displayHash))
	{
		if (STREAMING::IS_MODEL_IN_CD_IMAGE(modelHash))
		{
			while(!STREAMING::HAS_MODEL_LOADED(modelHash))
			{
				STREAMING::REQUEST_MODEL(modelHash);
				SYSTEM::WAIT(0);
			}
			Temp = PED::GET_VEHICLE_PED_IS_USING(PLAYER::PLAYER_PED_ID());
			if (ENTITY::DOES_ENTITY_EXIST(Temp))
			{
				VEHICLE::DELETE(&Temp);
			}
			Temp = VEHICLE::CREATE_VEHICLE(modelHash, ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(PED::PLAYER_PED_ID(), 0.0f, 4.0f, 1.0f), ENTITY::GET_ENTITY_HEADING(PLAYER::PLAYER_PED_ID()) + 90f, 0, 1);
			VEHICLE::SET_VEHICLE_ON_GROUND_PROPERLY(Temp);
			STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(modelHash);
			VEHICLE::SET_VEHICLE_AS_NO_LONGER_NEEDED(Temp);
		}
	}
}
*/
const char *Function = "\x2D\x02\x03\x00\x00\x38\x01\x2C\x05\x00\x1C\x56\x70\x00\x38\x00\x2C\x05\x00\x44\x56\x67\x00\x38\x00\x2C\x05\x00\x25\x06\x56\x0E\x00\x38\x00\x2C\x04\x00\x3D\x6E\x2C\x04\x00\x81\x55\xE8\xFF\x2C\x01\x00\x8F\x2C\x05\x00\x4B\x39\x04\x38\x04\x2C\x05\x00\x83\x56\x06\x00\x37\x04\x2C\x04\x00\x70\x38\x00\x2C\x01\x00\x8F\x77\x7B\x78\x2C\x13\x00\x2A\x2C\x01\x00\x8F\x2C\x05\x00\x4C\x29\x00\x00\xB4\x42\x0E\x6E\x6F\x2C\x1D\x00\x37\x39\x04\x38\x04\x2C\x05\x00\x65\x2B\x38\x00\x2C\x04\x00\x13\x37\x04\x2C\x04\x00\x0A\x2E\x02\x00";
int(*GetHashKey)(char*, unsigned int);

uintptr_t FindPattern(const char *pattern, const char *mask, const char* startAddress, size_t size)
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

uintptr_t FindPattern(const char *pattern, const char *mask)
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
	__int64 getHashKeyPattern = FindPattern("\x48\x8B\x0B\x33\xD2\xE8\x00\x00\x00\x00\x89\x03", "xxxxxx????xx");
	GetHashKey = reinterpret_cast<int(*)(char*, unsigned int)>(*reinterpret_cast<int*>(getHashKeyPattern + 6) + getHashKeyPattern + 10);

	if (!GlobalPattern)
	{
		Log::Msg("ERROR: finding address 0");
		Log::Msg("Aborting...");
		FreeLibraryAndExitThread(_hinstDLL, 0);
	}
	if (!ScriptPattern)
	{
		Log::Msg("ERROR: finding address 1");
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
	FindCheatController();
	EnableCarsGlobal();
	FreeLibraryAndExitThread(_hinstDLL, 0);
}

void EnableCars::FindShopController()
{
	for (int i = 0; i < 1000; i++)
	{
		if (*(int*)(ScriptBasePointer + (i << 4) + 12) == 0x39DA738B)
		{
			shopControllerAddress = (__int64*)(ScriptBasePointer + (i << 4));
			if (!*shopControllerAddress)
			{
				DEBUGMSG("shop controller script not loaded, waiting...");
			}
			while (!*shopControllerAddress)
			{
				Sleep(100);
			}
			DEBUGMSG("Shop Controller script loaded, Address = %llX", *shopControllerAddress);
			return;
		}
	}
	Log::Msg("ERROR: finding address 2");
	Log::Msg("Aborting...");
	FreeLibraryAndExitThread(_hinstDLL, 0);
}

void EnableCars::FindCheatController()
{
	for (int i = 0; i < 1000; i++)
	{
		if (*(int*)(ScriptBasePointer + (i << 4) + 12) == 0xAFD9916D)
		{
			cheatControllerAddress = (__int64*)(ScriptBasePointer + (i << 4));
			if (!*cheatControllerAddress)
			{
				DEBUGMSG("cheat controller script not loaded, waiting...");
			}
			while (!*cheatControllerAddress)
			{
				Sleep(100);
			}
			DEBUGMSG("Cheat Controller script loaded, Address = %llX", *cheatControllerAddress);
			return;
		}
	}
	Log::Msg("ERROR: finding address 3");
	Log::Msg("Aborting...");
	FreeLibraryAndExitThread(_hinstDLL, 0);
}


__int64 EnableCars::GetScriptAddress(__int64* codeBlocksOff, int offset)
{
	return *(codeBlocksOff + (offset >> 14)) + (offset & 0x3FFF);
}

void EnableCars::EnableCarsGlobal()
{
	__int64 scriptBaseAddress = *shopControllerAddress;
	int CodeLength = *(int*)(scriptBaseAddress + 0x1C);
	int codeBlocks = CodeLength >> 14;
	DEBUGMSG("Script Loaded, CodeLength = %d, CodeBlockCount = %d", CodeLength, codeBlocks);
	__int64* CodeBlockOffset = *(__int64**)(scriptBaseAddress + 0x10);
	for (int i = 0; i < codeBlocks; i++)
	{
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
								FindSwitch(CodeBlockOffset, RealCodeOff - j);
								PatchCheatController();
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
			Log::Msg("Error with car hash %X", *(int*)(caseOff + 1));// in reality this should never be executed
			continue;
		}
		char* displayName = GetDisplayName(hash);
		Log::Msg("Found affected hash: 0x%08X - DisplayName = %s", hash, displayName);
		HashPair h;
		h.DispHash = GetHashKey(displayName, 0);
		h.modelHash = hash;
		CarHashes.push_back(h);
	}
}
char* EnableCars::GetDisplayName(int modelHash)
{
	int data = 0xFFFF;
	__int64 addr = GetModelInfo(modelHash, (__int64)&data);
	if (addr && (*(unsigned char*)(addr + 157) & 0x1F) == 5)
	{
		return (char*)(addr + displayNameOffset);
	}
	else
	{
		return "CARNOTFOUND";
	}
}

void EnableCars::PatchCheatController()
{
	if (CarHashes.size() == 0)
	{
		return;//no car hashes found
	}
	Log::Msg("Applying patch to enable car spawning");
	__int64 scriptBaseAddress = *cheatControllerAddress;
	int *CodeLengthPtr = (int*)(scriptBaseAddress + 0x1C);
	int staticCount = *(int*)(scriptBaseAddress + 0x24);
	__int64 staticOff = *(__int64*)(scriptBaseAddress + 0x30);
	__int64 StartOff = (staticOff + staticCount * 8 + 15) & 0xFFFFFFFFFFFFFF00;
	DEBUGMSG("Cheat controller Loaded, CodeLength = %d", *CodeLengthPtr);
	__int64* CodeBlockOffset = *(__int64**)(scriptBaseAddress + 0x10);
	
	int *setOffset = NULL;
	for (int i = 0; i < *CodeLengthPtr;i++)
	{
		if (*(__int64*)GetScriptAddress(CodeBlockOffset, i) == 0x3C6E5C3C6E00002E)
		{
			setOffset = (int*)GetScriptAddress(CodeBlockOffset, i + 3);
			DEBUGMSG("Found cheat controller hook offset: %X", setOffset);
			break;
		}
	}
	if (!setOffset)
	{
		Log::Msg("Error applying patch, car spawning disabled");
		return;
	}
	DEBUGMSG("Extending code table");
	CodeBlockOffset[1] = (__int64)StartOff;
	*CodeLengthPtr = 0x408E + 0x14 * (int)CarHashes.size();
	DEBUGMSG("CodeTable Length = %X", *CodeLengthPtr);
	DEBUGMSG("Starting offset = %llX", StartOff);
	memcpy((void*)StartOff, (void*)Function, 129);//function takes model hash and display hash, returns void
	DEBUGMSG("Copied new function");
	unsigned char* cur = (unsigned char*)((__int64)StartOff + 129);
	memcpy((void*)cur, (void*)"\x2D\x00\x02\x00\x00", 5);//function for adding the new cheats and fixing patch, no params, no returns
	cur += 5;
	memcpy((void*)cur, setOffset, 6);//copy the code for setting the locals used to hook this function
	cur += 6;
	for (int i = 0, max = CarHashes.size(); i<max;i++)
	{
		*cur++ = 0x28;
		*(int*)cur = CarHashes[i].modelHash;//push model hash
		cur += 4;
		*cur++ = 0x28;
		*(int*)cur = CarHashes[i].DispHash;//push display hash
		cur += 4;
		*(int*)cur = 0x0040005D;//call our new function for checking if cheat entered, then spawning car
		cur += 4;
	}
	
	*(int*)cur = 0x0000002E;//return statement

	DEBUGMSG("Patching new function"); //0x0040805D
	memcpy((void*)setOffset, (void*)"\x5D\x80\x40\x00\x00\x00", 6);
	Log::Msg("Success, spawn the cars using their display name as a cheatcode (press ` in game)");
	
}