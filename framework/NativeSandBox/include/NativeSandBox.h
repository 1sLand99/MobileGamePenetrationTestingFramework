#ifndef NATIVESANDBOX_H
#define NATIVESANDBOX_H
#include <DebugHelper.h>
#include <stddef.h>
#include <stdint.h>
#include <string>
using namespace std;

enum{
    E_EntryType_HwExecBp,
    E_EntryType_registernatives,
    E_EntryType_callfunction,
    E_EntryType_myFunHandler_pthread_create,
    E_EntryType_dlsym,
    E_EntryType_pthread_create,
    E_EntryType_InlineHook
};

void* getEmuEntry(void *originFunctionAddr, size_t entryType);

size_t emuRunWrap(size_t x0, size_t x1, size_t x2, size_t x3, size_t x4,
                size_t x5, size_t x6, size_t x7, size_t originFuncSP,
                size_t originFuncAddr, size_t originFuncRet, size_t entryType,
                bool (*cb)(DebugHelper*dh),
                std::string debugHelperLogTag = "libMyHook");

void* initJmpTable();

void setOriginFunctionSPBuffer(size_t originFuncSP, uint64_t *arg8, size_t *arg9, size_t *arg10,
                          size_t *arg11, size_t *arg12, size_t *arg13, size_t *arg14,
                          size_t *arg15, size_t *arg16, size_t *arg17, size_t *arg18,
                          size_t *arg19, size_t *arg20);
#endif