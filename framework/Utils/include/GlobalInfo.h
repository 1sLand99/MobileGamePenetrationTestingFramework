#ifndef GLOBALINFO_H
#define GLOBALINFO_H
#include <string>

#define enableTempLog 1

#define MySpeed 5

#define isHwEmu 0
#define SIG_HWBP 44

#define isEmu 1

#define analysisSoName "libslua.so"
#define luaSoName  "libxlua.so"
#define testSoName    "libmyapplication.so"
#define il2cppSoName  "libil2cpp.so"
#define unitySoName  "libunity.so"

#define registerLua
// #define dumpLua
// #define logLua
#define injectLua
// #define replaceLua
// #define traceLua

#define setGTabLuaScriptName "@function.lua"
#define registerCFuncLuaScriptName "@function.lua"

#define injectLuaScriptName "@function.lua"
#define injectLuaScriptBuff ""

#define replaceLuaScriptName "@function.lua"
#define replaceLuaScriptBuff ""

#endif