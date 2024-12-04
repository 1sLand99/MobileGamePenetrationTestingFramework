#include <pthread.h>
#include <GlobalInfo.h>
#include <Log.h>
#include <sys/mman.h>
#include <Util.h>
#include <HwBreakpointManager.h>

static pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER; //虽然也是全局变量，但只能在本文件使用.
static void *jmpTable = 0;

void *initJmpTable()
{
    jmpTable = mmap(0, 0x20000, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    return jmpTable;
}

void *getEmuEntry(void *originFunctionAddr, size_t entryType)
{
    LOGI("getEmuEntry start");
    if(jmpTable == 0)
    {
        jmpTable = initJmpTable();
    }
    int isLock = 0;
    isLock = pthread_mutex_lock(&myMutex);
    if (isLock == 0)
    {
        void *emuEntryAddress = jmpTable;
        *(size_t *)jmpTable = (size_t)emuRunWrap;                         // 放 emuRunWrap 地址
        *(size_t *)((size_t)jmpTable + 0x8) = (size_t)originFunctionAddr; // 原函数地址
        *(uint32_t *)((size_t)jmpTable +
                      0x10) = (uint32_t)0x10000009; // 09 00 00 10 == adr x9, .                  读取当前PC.
        *(uint32_t *)((size_t)jmpTable +
                      0x14) = (uint32_t)0x910003EE; // EE 03 00 91 == mov x14, sp                 >>> x14 -----> sp <<<
        *(uint32_t *)((size_t)jmpTable +
                      0x18) = (uint32_t)0xF85F812F; // 2F 81 5F F8 == ldr x15, [x9, #-0x8]        >>> x15 -----> 原函数地址 <<<
        *(uint32_t *)((size_t)jmpTable +
                      0x1C) = (uint32_t)0xF85F012C; // 2C 01 5F F8 == ldr x12, [x9, #-0x10].      >>> x12 -----> emuRunWrap <<<
        *(uint32_t *)((size_t)jmpTable +
                      0x20) = (uint32_t)0xD10083FF; // FF 83 00 D1 == sub sp, sp, 0x20
        *(uint32_t *)((size_t)jmpTable +
                      0x24) = (uint32_t)0xF90003EE; // EE 03 00 F9 == str x14, [sp]
        *(uint32_t *)((size_t)jmpTable +
                      0x28) = (uint32_t)0xF90007EF; // EF 07 00 F9 == str x15, [sp, #0x8]
        *(uint32_t *)((size_t)jmpTable +
                      0x2C) = (uint32_t)0xF9000BFE; // FE 0B 00 F9 == str lr, [sp, #0x10]        >>>> 保存 lr <<<<
        /***************************************************************************************************************************************************/
        // 这里思考下能不能传一个值给他, 然后在 emuRunWrap内部做判断, 如果值为1, 说明是dlsym过来的, 如果是2, 则是 registerNative过来的, 如果是2, 则是 pthread_create过来的,
        if (entryType == E_EntryType_HwExecBp)
        { // None
            *(uint32_t *)((size_t)jmpTable +
                          0x30) = (uint32_t)0xD2800009; // 09 00 80 D2 == mov x9, 0
        }
        else if (entryType == E_EntryType_registernatives)
        { // registernatives
            *(uint32_t *)((size_t)jmpTable +
                          0x30) = (uint32_t)0xD2800029; // 29 00 80 D2 == mov x9, 1
        }
        else if (entryType == E_EntryType_callfunction)
        { // callfunction
            *(uint32_t *)((size_t)jmpTable +
                          0x30) = (uint32_t)0xD2800049; // 49 00 80 D2 == mov x9, 2
        }
        else if (entryType == E_EntryType_myFunHandler_pthread_create)
        { // myFunHandler -> pthread_create
            *(uint32_t *)((size_t)jmpTable +
                          0x30) = (uint32_t)0xD2800069; // 69 00 80 D2 == mov x9, 3
        }
        else if (entryType == E_EntryType_dlsym)
        { // dlsym
            *(uint32_t *)((size_t)jmpTable +
                          0x30) = (uint32_t)0xD2800089; // 89 00 80 D2 == mov x9, 4
        }
        else if (entryType == E_EntryType_pthread_create)
        { // pthread-create
            *(uint32_t *)((size_t)jmpTable +
                          0x30) = (uint32_t)0xD28000A9; // A9 00 80 D2 == mov x9, 5
        }
        else if (entryType == E_EntryType_InlineHook)
        { // pthread-create
            *(uint32_t *)((size_t)jmpTable +
                          0x30) = (uint32_t)0xD28000C9; // C9 00 80 D2 == mov x9, 6
        }
        /***************************************************************************************************************************************************/
        *(uint32_t *)((size_t)jmpTable +
                      0x34) = (uint32_t)0xF9000FE9; // E9 0F 00 F9 == str x9, [sp, #0x18]
        *(uint32_t *)((size_t)jmpTable +
                      0x38) = (uint32_t)0xD63F0180; // 80 01 3F D6 == blr x12; 跳到 emuRunWrap
        *(uint32_t *)((size_t)jmpTable +
                      0x3C) = (uint32_t)0xF9400BFE; // FE 0B 40 F9 == ldr lr, [sp, #0x10]        >>>> 取出 lr <<<<
        *(uint32_t *)((size_t)jmpTable +
                      0x40) = (uint32_t)0x910083FF;                    // FF 83 00 91 == add sp, sp, 0x20
        *(uint32_t *)((size_t)jmpTable + 0x44) = (uint32_t)0xD65F03C0; // C0 03 5F D6 == ret
        jmpTable = (void *)((size_t)jmpTable + 0x48);
        emuEntryAddress = (void *)((size_t)emuEntryAddress + 0x10);
        pthread_mutex_unlock(&myMutex);
        LOGI("getEmuEntry finish");
        return emuEntryAddress;
    }
    return (void *)-1;
}

// 存在意义：这个函数是跳板.
size_t emuRunWrap(size_t x0, size_t x1, size_t x2, size_t x3, size_t x4,
                  size_t x5, size_t x6, size_t x7, size_t originFuncSP,
                  size_t originFuncAddr, size_t originFuncRet, size_t entryType,
                  bool (*cb)(DebugHelper*dh), std::string debugHelperLogTag)
{
    LOGI("emuRunWrap start x0:%zx x1:%zx x2:%zx x3:%zx x4:%zx x5:%zx x6:%zx x7:%zx sp:%zx func:%zx ret:%zx et:%zx",
         x0, x1, x2, x3, x4, x5, x6, x7, originFuncSP, originFuncAddr, originFuncRet, entryType);
    Emulator emu;
    Dl_info originFunctionDLInfo = MyUtil::getAddrDLInfo((void *)originFuncAddr);
    std::string emuRunWrapLogTag = "emuRunWrap-";
    if (entryType == E_EntryType_InlineHook)
    {
        emuRunWrapLogTag += "InlineHook-";
    }
    else if (entryType == E_EntryType_HwExecBp)
    {
        emuRunWrapLogTag += "HwExecBp-";
    }
    else if (entryType == E_EntryType_registernatives)
    {
        emuRunWrapLogTag += "registernatives-";
    }
    else if (entryType == E_EntryType_callfunction)
    {
        emuRunWrapLogTag += "callfunction-";
    }
    else if (entryType == E_EntryType_myFunHandler_pthread_create)
    {
        emuRunWrapLogTag += "pthread_create-";
    }
    else if (entryType == E_EntryType_dlsym)
    {
        emuRunWrapLogTag += "dlsym-";
    }
    else if (entryType == E_EntryType_pthread_create)
    {
        emuRunWrapLogTag += "pthread_create_libc-";
    }
    DebugHelper helper(&emu, debugHelperLogTag.c_str(), true);
    cb(&helper);
    // helper.addTraceRange(-1);
    size_t x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, ret = 0;
    setOriginFunctionSPBuffer(originFuncSP, &x8, &x9, &x10, &x11, &x12, &x13, &x14,
                              &x15, &x16, &x17, &x18, &x19, &x20);
    //非硬件断点进入的流程.
    if (entryType != E_EntryType_HwExecBp)
    {
        if (isEmu)
        {
            // [1] 进入 emu.run 让函数被模拟执行.
            LOGI("Emu start: [sname:%s] [Start:%p] [Ret:%p] ", originFunctionDLInfo.dli_sname, originFuncAddr, originFuncRet);
            emu.run(&ret, 0, (void *)originFuncAddr, 20, x0, x1, x2, x3, x4, x5, x6, x7,
                    x8, x9, x10, x11, x12, x12, x13, x14, x15, x16, x17, x18, x19, x20);
            LOGI("Emu end: [sname:%s] [Start:%p] [Ret:%p] ", originFunctionDLInfo.dli_sname, originFuncAddr, originFuncRet);
        }
        else
        {
            // [2] 不进入 emu.run 直接执行函数. SKIP
            LOGI("Emu skip start : [sname:%s] [base:%p] [Start:%p] [Ret:%p] ", originFunctionDLInfo.dli_sname, originFunctionDLInfo.dli_fbase,
             originFuncAddr, originFuncRet);
            ret = sp_copy_call64(x0, x1, x2, x3, x4, x5, x6, x7, originFuncSP,
                                 (void *)originFuncAddr, 20);
            LOGI("Emu skip end : [sname:%s] [Start:%p] [Ret:%p] ", originFunctionDLInfo.dli_sname, originFuncAddr, originFuncRet);
        }
    }
    else
    {
        LOGI("硬件断点模拟执行 开始");
        addHardwareBreakpoint( (size_t) originFuncAddr,  HW_BREAKPOINT_LEN_4, HW_BREAKPOINT_X, 0,
                                0, 0, 0, 0,
                                0, 0, HW_BREAKPOINT_X, 0);
        emu.run(&ret, 0, (void *)originFuncAddr, 20, x0, x1, x2, x3, x4, x5, x6, x7,
                x8, x9, x10, x11, x12, x12, x13, x14, x15, x16, x17, x18, x19, x20);
        LOGI("硬件断点模拟执行 结束");
    }
    return ret;
}

void setOriginFunctionSPBuffer(size_t originFuncSP, uint64_t *arg8, size_t *arg9, size_t *arg10,
                               size_t *arg11, size_t *arg12, size_t *arg13, size_t *arg14,
                               size_t *arg15, size_t *arg16, size_t *arg17, size_t *arg18,
                               size_t *arg19, size_t *arg20)
{
    *arg8 = *(size_t *)(originFuncSP + 0x0);
    *arg9 = *(size_t *)(originFuncSP + 0x8);
    *arg10 = *(size_t *)(originFuncSP + 0x10);
    *arg11 = *(size_t *)(originFuncSP + 0x18);
    *arg12 = *(size_t *)(originFuncSP + 0x20);
    *arg13 = *(size_t *)(originFuncSP + 0x28);
    *arg14 = *(size_t *)(originFuncSP + 0x30);
    *arg15 = *(size_t *)(originFuncSP + 0x38);
    *arg16 = *(size_t *)(originFuncSP + 0x40);
    *arg17 = *(size_t *)(originFuncSP + 0x48);
    *arg18 = *(size_t *)(originFuncSP + 0x50);
    *arg19 = *(size_t *)(originFuncSP + 0x58);
    *arg20 = *(size_t *)(originFuncSP + 0x60);
    return;
}