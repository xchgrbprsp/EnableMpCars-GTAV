#pragma once



class EnableCars
{
public:
	static void Run(HINSTANCE hinstDLL);
private:
	static void FindPatterns();
	static uintptr_t FindPattern(const char *pattern, const char *mask, const char* startAddress, size_t size);
	static uintptr_t FindPattern(const char *pattern, const char *mask);
	static void FindGlobalAddress();//could be done using shv
	static void FindScriptAddresses();
	static __int64 *getGlobalAddress(int index);//could be done using shv
	static void FindShopController();
	static void EnableCarsGlobal();
	static __int64 GetScriptAddress(__int64* codeBlocksOff, int offset);


	//not essential
	static void FindAffectedCars(__int64* codeBlockOffset, int scriptSwitchOffset);
	static void FindSwitch(__int64* codeBlockOffset, int funcCallOffset);
	static char* GetDisplayName(int modelHash);

};
