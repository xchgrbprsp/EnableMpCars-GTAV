#pragma once



class EnableCars
{
public:
	static void Run(HINSTANCE hinstDLL);
private:
	static void FindPatterns();
	static void FindGlobalAddress();//could be done using shv
	static void FindScriptAddresses();
	static __int64 *getGlobalAddress(int index);//could be done using shv
	static void FindShopController();
	static void FindCheatController();
	static void EnableCarsGlobal();
	static __int64 GetScriptAddress(__int64* codeBlocksOff, int offset);
	static void PatchCheatController();


	//not essential
	static void FindAffectedCars(__int64* codeBlockOffset, int scriptSwitchOffset);
	static void FindSwitch(__int64* codeBlockOffset, int funcCallOffset);
	static char* GetDisplayName(int modelHash);

};
