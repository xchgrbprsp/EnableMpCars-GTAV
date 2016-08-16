#pragma once
#include <cstddef>

struct ScriptHeader
{
	char padding1[16];					//0x0
	unsigned char** codeBlocksOffset;	//0x10
	char padding2[4];					//0x18
	int codeLength;						//0x1C
	char padding3[4];					//0x20
	int localCount;						//0x24
	char padding4[4];					//0x28
	int nativeCount;					//0x2C
	__int64* localOffset;				//0x30
	char padding5[8];					//0x38
	__int64* nativeOffset;				//0x40
	char padding6[16];					//0x48
	int nameHash;						//0x58
	char padding7[4];					//0x5C
	char* name;							//0x60
	char** stringsOffset;				//0x68
	int stringSize;						//0x70
	char padding8[12];					//0x74
	//END_OF_HEADER

	inline bool IsValid() const { return codeLength > 0; }
	inline int CodePageCount() const { return (codeLength + 0x3FFF) >> 14; }
	inline int GetCodePageSize(int page) const
	{
		return (page < 0 || page >= CodePageCount() ? 0 : (page == CodePageCount() - 1) ? codeLength & 0x3FFF : 0x4000);
	}
	inline unsigned char* GetCodePageAddress(int page) const{ return codeBlocksOffset[page]; }
	inline unsigned char* GetCodePositionAddress(int codePosition) const
	{
		return codePosition < 0 || codePosition >= codeLength ? NULL : &codeBlocksOffset[codePosition >> 14][codePosition & 0x3FFF];
	}
	inline char* GetString(int stringPosition)const
	{
		return stringPosition < 0 || stringPosition >= stringSize ? NULL : &stringsOffset[stringPosition >> 14][stringPosition & 0x3FFF];
	}

};
struct ScriptTableItem
{
public:
	ScriptHeader* Header;
	char padding[4];
	int hash;

	inline bool IsLoaded() const
	{
		return Header != NULL;
	}
};

struct ScriptTable
{
	ScriptTableItem* TablePtr;
	char padding[16];
	int count;

public:
	ScriptTableItem* FindScript(int hash)
	{
		if (TablePtr == NULL)
		{
			return NULL;//table initialisation hasnt happened yet
		}
		for (int i = 0; i<count;i++)
		{
			if (TablePtr[i].hash == hash)
			{
				return &TablePtr[i];
			}
		}
		return NULL;
	}
};

struct GlobalTable
{
	__int64** GlobalBasePtr;
	inline __int64* AddressOf(int index) const{	return &GlobalBasePtr[index >> 18][index & 0x3FFFF];}
	inline bool IsInitialised()const{ return *GlobalBasePtr != NULL; }
};