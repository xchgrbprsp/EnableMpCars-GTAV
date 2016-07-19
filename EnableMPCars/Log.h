#pragma once

#ifdef __DEBUG
#define DEBUGOUT( X, ... ) Log::Debug( X, __VA_ARGS__ )
#else
#define DEBUGOUT( X, ... )
#endif

#if _DEBUG
#define DEBUGMSG Log::Debug
#else
#define DEBUG_MSG2 /
#define DEBUGMSG /DEBUG_MSG2
#endif

class Log
{
public:
	static void Init(HMODULE hModule);
#ifdef _DEBUG
	static void Debug(const char* fmt, ...);
#endif
	static void Msg(const char* fmt, ...);
};
