#include "stdafx.h"
#include <Psapi.h>
#include <vector>
#include "Structs.h"

HINSTANCE _hinstDLL;

struct HashPair
{
	int modelHash;
	int DispHash;
};
std::vector<HashPair> CarHashes;



ScriptTable* scriptTable;
GlobalTable globalTable;
ScriptHeader* cheatController;
ScriptHeader* shopController;


__int64(*GetModelInfo)(int, __int64);//used to extract display names without actually calling natives
int displayNameOffset;//the offset where the display name is stored relative to the address received above changes per game build, so its safer to read the offset from a signature
int(*GetHashKey)(char*, unsigned int);//just use the in game hashing function to save writing my own

/*dont even bother trying to understand this function code
Its low level ysc assembley code hacked to work with the cheat_controller.ysc script with doesnt significatly change per game update(few globals shifted is about it)
void SpawnCarCheck(Hash modelHash, Hash displayHash)
{
	Vehicle Temp;
	if (GAMEPLAY::_HAS_CHEAT_STRING_JUST_BEEN_ENTERED(displayHash))
	{
		if (STREAMING::IS_MODEL_IN_CD_IMAGE(modelHash)) //this isnt strictly required as we check models for validity earlier, but it's best to be triply safe.
		{
			while(!STREAMING::HAS_MODEL_LOADED(modelHash))
			{
				STREAMING::REQUEST_MODEL(modelHash);
				SYSTEM::WAIT(0);
			}
			Temp = PED::GET_VEHICLE_PED_IS_USING(PLAYER::PLAYER_PED_ID());
			if (ENTITY::DOES_ENTITY_EXIST(Temp))
			{
				VEHICLE::DELETE_VEHICLE(&Temp);
			}
			Temp = VEHICLE::CREATE_VEHICLE(modelHash, ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(PED::PLAYER_PED_ID(), 0.0f, 4.0f, 1.0f), ENTITY::GET_ENTITY_HEADING(PLAYER::PLAYER_PED_ID()) + 90f, 0, 1);
			VEHICLE::SET_VEHICLE_ON_GROUND_PROPERLY(Temp);
			STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(modelHash);
			VEHICLE::SET_VEHICLE_AS_NO_LONGER_NEEDED(&Temp);
		}
	}
}
*/
const char *Function = "\x2D\x02\x03\x00\x00\x38\x01\x2C\x05\x00\x1C\x56\x70\x00\x38\x00\x2C\x05\x00\x44\x56\x67\x00\x38\x00\x2C\x05\x00\x25\x06\x56\x0E\x00\x38\x00\x2C\x04\x00\x3D\x6E\x2C\x04\x00\x81\x55\xE8\xFF\x2C\x01\x00\x8F\x2C\x05\x00\x4B\x39\x04\x38\x04\x2C\x05\x00\x83\x56\x06\x00\x37\x04\x2C\x04\x00\x70\x38\x00\x2C\x01\x00\x8F\x77\x7B\x78\x2C\x13\x00\x2A\x2C\x01\x00\x8F\x2C\x05\x00\x4C\x29\x00\x00\xB4\x42\x0E\x6E\x6F\x2C\x1D\x00\x37\x39\x04\x38\x04\x2C\x05\x00\x65\x2B\x38\x00\x2C\x04\x00\x13\x37\x04\x2C\x04\x00\x0A\x2E\x02\x00";

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

void EnableCars::FindScriptAddresses()
{

	while(*(__int64*)scriptTable == 0)
	{
		Sleep(100);
	}
	DEBUGMSG("Found script base pointer %llX", (__int64)scriptTable);
	ScriptTableItem* Item = scriptTable->FindScript(0x39DA738B);
	if(Item == NULL)
	{
		Log::Msg("ERROR: finding address 2");
		Log::Msg("Aborting...");
		FreeLibraryAndExitThread(_hinstDLL, 0);
	}
	while(!Item->IsLoaded())
	{
		Sleep(100);
	}
	shopController = Item->Header;

	Item = scriptTable->FindScript(0xAFD9916D);
	if(Item == NULL)
	{
		Log::Msg("ERROR: finding address 3");
		Log::Msg("Aborting...");
		FreeLibraryAndExitThread(_hinstDLL, 0);
	}
	while(!Item->IsLoaded())
	{
		Sleep(100);
	}
	cheatController = Item->Header;
}

void EnableCars::FindPatterns()
{
	__int64 patternAddr = FindPattern("\x4C\x8D\x05\x00\x00\x00\x00\x4D\x8B\x08\x4D\x85\xC9\x74\x11", "xxx????xxxxxxxx");
	if(!patternAddr)
	{
		Log::Msg("ERROR: finding address 0");
		Log::Msg("Aborting...");
		FreeLibraryAndExitThread(_hinstDLL, 0);
	}
	globalTable.GlobalBasePtr = (__int64**)(patternAddr + *(int*)(patternAddr + 3) + 7);


	patternAddr = FindPattern("\x48\x03\x15\x00\x00\x00\x00\x4C\x23\xC2\x49\x8B\x08", "xxx????xxxxxx");
	if(!patternAddr)
	{
		Log::Msg("ERROR: finding address 1");
		Log::Msg("Aborting...");
		FreeLibraryAndExitThread(_hinstDLL, 0);
	}
	scriptTable = (ScriptTable*)(patternAddr + *(int*)(patternAddr + 3) + 7);


	patternAddr = FindPattern("\x80\xF9\x05\x75\x08\x48\x05\x00\x00\x00\x00", "xxxxxxx????");
	GetModelInfo = (__int64(*)(int, __int64))(*(int*)(patternAddr - 0x12) + patternAddr - 0x12 + 0x4);
	displayNameOffset = *(int*)(patternAddr + 0x7);
	__int64 getHashKeyPattern = FindPattern("\x48\x8B\x0B\x33\xD2\xE8\x00\x00\x00\x00\x89\x03", "xxxxxx????xx");
	GetHashKey = reinterpret_cast<int(*)(char*, unsigned int)>(*reinterpret_cast<int*>(getHashKeyPattern + 6) + getHashKeyPattern + 10);

	while(!globalTable.IsInitialised())Sleep(100);//Wait for GlobalInitialisation before continuing
	DEBUGMSG("Found global base pointer %llX", (__int64)globalTable.GlobalBasePtr);	
}


void EnableCars::Run(HINSTANCE hinstDLL)
{
	_hinstDLL = hinstDLL;
	Log::Init(hinstDLL);
	FindPatterns();
	FindScriptAddresses();
	EnableCarsGlobal();
	FreeLibraryAndExitThread(_hinstDLL, 0);
}


void EnableCars::EnableCarsGlobal()
{
	for (int i = 0; i < shopController->CodePageCount(); i++)
	{
		__int64 sigAddress = FindPattern("\x28\x26\xCE\x6B\x86\x39\x03", "xxxxxxx", (const char*)shopController->GetCodePageAddress(i), shopController->GetCodePageSize(i));
		if (!sigAddress)
		{
			continue;
		}
		DEBUGMSG("Pattern Found in codepage %d at memory address %llX", i, sigAddress);
		int RealCodeOff = (int)(sigAddress - (__int64)shopController->GetCodePageAddress(i) + (i << 14));
		for (int j = 0; j < 2000; j++)
		{
			if (*(int*)shopController->GetCodePositionAddress(RealCodeOff - j) == 0x0008012D)
			{
				int funcOff = *(int*)shopController->GetCodePositionAddress(RealCodeOff - j + 6) & 0xFFFFFF;
				DEBUGMSG("found Function codepage address at %x", funcOff);
				for (int k = 0x5; k < 0x40; k++)
				{
					if ((*(int*)shopController->GetCodePositionAddress(funcOff + k) & 0xFFFFFF) == 0x01002E)
					{
						for (k = k + 1; k < 30; k++)
						{
							if (*(unsigned char*)shopController->GetCodePositionAddress(funcOff + k) == 0x5F)
							{
								int globalindex = *(int*)shopController->GetCodePositionAddress(funcOff + k + 1) & 0xFFFFFF;
								DEBUGMSG("Found Global Variable %d, address = %llX", globalindex, (__int64)globalTable.AddressOf(globalindex));
								Log::Msg("Setting Global Variable %d to true", globalindex);
								*globalTable.AddressOf(globalindex) = 1;
								Log::Msg("MP Cars enabled");
								FindSwitch(RealCodeOff - j);
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

void EnableCars::FindSwitch(int funcCallOffset)
{
	for (int i = 14; i<400;i++)
	{
		if (*(unsigned char*)shopController->GetCodePositionAddress(funcCallOffset + i) == 0x62)
		{
			FindAffectedCars(funcCallOffset + i);
			return;
		}
	}
	Log::Msg("Couldnt find affected cars");
}

void EnableCars::FindAffectedCars(int scriptSwitchOffset)
{
	__int64 curAddress = (__int64)shopController->GetCodePositionAddress(scriptSwitchOffset + 2);
	int startOff = scriptSwitchOffset + 2;
	int cases = *(unsigned char*)(curAddress - 1);
	for (int i = 0; i<cases;i++)
	{
		curAddress += 6;
		startOff += 6;
		int jumpoff = *(short*)(curAddress - 2);
		__int64 caseOff = (__int64)shopController->GetCodePositionAddress(startOff + jumpoff);
		unsigned char opCode = *(unsigned char*)caseOff;
		int hash;
		if (opCode == 0x28)//push int
		{
			hash = *(int*)(caseOff + 1);
		}
		else if(opCode == 0x61)//push int 24
		{
			hash = *(int*)(caseOff + 1) & 0xFFFFFF;
		}
		else
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
	if (addr && (*(unsigned char*)(addr + 157) & 0x1F) == 5)//make sure model is valid and is a car
	{
		return (char*)(addr + displayNameOffset);
	}
	return "CARNOTFOUND";
}

void EnableCars::PatchCheatController()
{
	if (CarHashes.size() == 0)
	{
		return;//no car hashes found
	}
	Log::Msg("Applying patch to enable car spawning");
	__int64 StartOff = ((__int64)(cheatController->localOffset + cheatController->localCount)+ 15) & 0xFFFFFFFFFFFFFFF0;
	DEBUGMSG("Cheat controller Loaded, CodeLength = %d", cheatController->codeLength);
	
	int *setOffset = NULL;
	for (int i = 0; i < cheatController->codeLength;i++)
	{
		if (*(__int64*)cheatController->GetCodePositionAddress(i) == 0x3C6E5C3C6E00002E)//searching for the address of code in the ysc where we will hook our new function
		{
			setOffset = (int*)cheatController->GetCodePositionAddress(i + 3);//the address points to the code for  {iLocal_92 = 0; iLocal_93 = 0;} this code will get overwritten to hook into our new function that checks the new cheats we will add in
			DEBUGMSG("Found cheat controller hook offset: %X", setOffset);
			break;
		}
	}
	if (!setOffset)
	{
		Log::Msg("Error applying patch, car spawning disabled"); //this will be thrown if this asi gets injected twice for whatever reason
		return;
	}

	//this is probably very dangerous code as were extending the code table by just adding a new page, meaning other data in the ysc will be able to be though of as being part of the code table
	//however control flow will never reach that undefined part of the code table which is why this doesnt cause any error
	DEBUGMSG("Extending code table");
	cheatController->codeBlocksOffset[1] = (unsigned char*)StartOff;
	cheatController->codeLength = 0x408E + 14 * (int)CarHashes.size();
	DEBUGMSG("CodeTable Length = %X", cheatController->codeLength);
	DEBUGMSG("Starting offset = %llX", StartOff);
	memcpy((void*)StartOff, (void*)Function, 129);//function takes model hash and display hash, returns void
	DEBUGMSG("Copied new function");
	unsigned char* cur = (unsigned char*)((__int64)StartOff + 129);
	memcpy((void*)cur, (void*)"\x2D\x00\x02\x00\x00", 5);//function for adding the new cheats and fixing patch, no params, no returns
	cur += 5;
	//our hook overrides the {iLocal_92 = 0; iLocal_93 = 0;} code from above, so we need to copy that code into the start of our new hooked function to ensure the cheat controller behaves normally
	memcpy((void*)cur, setOffset, 6);
	cur += 6;
	for (size_t i = 0, max = CarHashes.size(); i<max;i++)
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
	
	*(int*)cur = 0x0000002E;//return statement, strinctly only needs to be 2E 00 00, but the extra makes the copy faster and that last 00 is just a nop so nothing will change.

	DEBUGMSG("Patching new function");
	//This is where we patch in the hook, its safest to do this last, just on the off chance the cheat controller tries to execute the new function before we have finished writing it (due to running on a different thread).
	memcpy((void*)setOffset, (void*)"\x5D\x80\x40\x00\x00\x00", 6);
	Log::Msg("Success, spawn the cars using their display name as a cheatcode (press ` in game)");
	
}
