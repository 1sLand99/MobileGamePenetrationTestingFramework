#ifndef UTIL_H
#define UTIL_H
#include <dlfcn.h>
#include <vector>
using namespace std;

namespace MyUtil
{
    // 定义一个结构体来保存所有寄存器的值
    typedef struct
    {
        uint64_t x[31];  // x0 - x30
        uint64_t sp;     // stack pointer
        uint64_t pc;     // program counter
        uint64_t pstate; // processor state
    } arm64_registers_t;

    extern "C" __attribute__((naked)) void get_arm64_registers(arm64_registers_t *regs);
    void hexdump(size_t ptr, int buflen);
    void allocJmptableMemory(void **ptrJmpTableMemory);
    Dl_info getAddrDLInfo(void *addr);
    void getProcessName(char *processName);
    bool containsByteSequence(uint64_t address, const std::vector<unsigned char> &sequence);
    /*内存扫描*/
    void sleepThread(int seconds);
    void printAllThreads(const char *target_name);
    bool startsWith(const std::string &str, const std::string &prefix);
    void searchMemory(const std::string &targetLibrary, const std::string &dumpDir);
    void searchMemory(const std::string &stringToSearch, const std::vector<std::string> &disabledPathsPrefix, const std::string &inPermission, std::function<void(uintptr_t, const std::string &, bool *)> callback, bool isLog);
    void searchMemory(const std::vector<uint8_t> &bytesToSearch, const std::vector<std::string> &disabledPathsPrefix, const std::string &inPermission, std::function<void(uintptr_t, const std::string &, bool *)> callback, bool isLog);
    bool isQualifiedMemoryPattern(const std::string &permissions, const std::string &inPermission);
    bool isMemoryReadable(uintptr_t startAddress, size_t size); /* 这个好像是用来检测DD dump的 */
    void dumpMemory(void *ptr, size_t size, const std::string &filename, const std::string &folder);
    void createDirectories(std::string &path);
    char *strcasestr(const char *haystack, const char *needle);
    void setupSignalHandler(size_t sigNo);
    void signalHandler(int signum, siginfo_t *info, void *context);
}
#endif
