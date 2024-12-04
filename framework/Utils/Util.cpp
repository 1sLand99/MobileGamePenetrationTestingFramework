#include <pthread.h>
#include <ctype.h>
#include <Util.h>
#include <Log.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <functional>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <PrintStack.h>

namespace MyUtil
{

    char *strcasestr(const char *haystack, const char *needle)
    {
        if (!haystack || !needle)
        {
            return NULL;
        }
        if (!*needle)
        {
            return (char *)haystack;
        }
        for (; *haystack; haystack++)
        {
            const char *h = haystack;
            const char *n = needle;
            while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n))
            {
                h++;
                n++;
            }
            if (!*n)
            {
                return (char *)haystack;
            }
        }
        return NULL;
    }

    extern "C" __attribute__((naked)) void get_arm64_registers(arm64_registers_t *regs)
    {
        asm volatile(
            "stp x0, x1, [x0, #0]\n"
            "stp x2, x3, [x0, #16]\n"
            "stp x4, x5, [x0, #32]\n"
            "stp x6, x7, [x0, #48]\n"
            "stp x8, x9, [x0, #64]\n"
            "stp x10, x11, [x0, #80]\n"
            "stp x12, x13, [x0, #96]\n"
            "stp x14, x15, [x0, #112]\n"
            "stp x16, x17, [x0, #128]\n"
            "stp x18, x19, [x0, #144]\n"
            "stp x20, x21, [x0, #160]\n"
            "stp x22, x23, [x0, #176]\n"
            "stp x24, x25, [x0, #192]\n"
            "stp x26, x27, [x0, #208]\n"
            "stp x28, x29, [x0, #224]\n"
            "str x30, [x0, #240]\n" // x30 (link register)
            "mov x1, sp\n"
            "str x1, [x0, #248]\n" // stack pointer
            "adr x1, .\n"
            "str x1, [x0, #256]\n" // program counter
            "mrs x1, nzcv\n"
            "str x1, [x0, #264]\n" // processor state
            "ret\n");
    }

    void sleepThread(int seconds)
    {
        LOGI("Sleeping for %d seconds\n", seconds);
#if defined(__aarch64__)
        void *pc;
        asm volatile("adr %0, ." : "=r"(pc));
        LOGI("Current PC Address: %p", pc);
#endif
        struct timespec req;
        req.tv_sec = seconds;
        req.tv_nsec = 0;
#if defined(__aarch64__)
        asm volatile(
            "mov x0, %0\n" // req structure pointer
            "mov x1, 0\n"  // NULL
            "mov x8, %1\n" // syscall number for nanosleep
            "svc 0\n"      // make the syscall
            :              // no output
            : "r"(&req), "I"(__NR_nanosleep)
            : "x0", "x1", "x8");
#endif
        LOGI("Woke up after %d seconds\n", seconds);
    }

    int suspend_thread(pid_t tid)
    {
        return syscall(SYS_tgkill, getpid(), tid, SIGSTOP);
    }

    void get_thread_name(pid_t tid, char *name, size_t len)
    {
        char path[256];
        snprintf(path, sizeof(path), "/proc/self/task/%d/comm", tid);
        FILE *status_file = fopen(path, "r");
        if (status_file)
        {
            if (fgets(name, len, status_file) != NULL)
            {
                name[strcspn(name, "\n")] = 0; // 去除换行符
            }
            fclose(status_file);
        }
        else
        {
            snprintf(name, len, "Unknown");
        }
        return;
    }

    void printAllThreads(const char *target_name)
    {
        DIR *dir;
        struct dirent *ent;
        char path[256];
        char thread_name[256];
        char map_path[256];
        FILE *status_file;
        pid_t tid;
        pid_t current_tid = syscall(SYS_gettid);
        pid_t main_tid = getpid();
        char current_thread_name[256];
        get_thread_name(current_tid, current_thread_name, sizeof(current_thread_name));
        LOGI("Current Thread ID: %d, Name: %s\n", current_tid, current_thread_name);
        if (!(dir = opendir("/proc")))
        {
            LOGE(" couldn't open /proc");
            return;
        }
        while ((ent = readdir(dir)) != NULL)
        {
            if (isdigit(ent->d_name[0]))
            {
                snprintf(path, sizeof(path), "/proc/%s/task", ent->d_name);
                DIR *task_dir;
                if (!(task_dir = opendir(path)))
                {
                    LOGE("couldn't open task directory");
                    continue;
                }
                while ((ent = readdir(task_dir)) != NULL)
                {
                    if (isdigit(ent->d_name[0]))
                    {
                        tid = atoi(ent->d_name);
                        snprintf(map_path, sizeof(map_path), "/proc/%s/task/%d/comm", ent->d_name, tid);
                        if ((status_file = fopen(map_path, "r")) != NULL)
                        {
                            if (fgets(thread_name, sizeof(thread_name), status_file) != NULL)
                            {
                                thread_name[strcspn(thread_name, "\n")] = 0;
                                LOGI("Thread ID: %d, Name: %s", tid, thread_name);
                                if (strcmp(thread_name, target_name) == 0 && tid != current_tid && tid != main_tid)
                                {
                                    LOGI("Suspending thread ID: %d %s", tid, thread_name);
                                    // suspend_thread(tid);
                                }
                            }
                            fclose(status_file);
                        }
                    }
                }
                closedir(task_dir);
            }
        }
        closedir(dir);
        return;
    }

    bool startsWith(const std::string &str, const std::string &prefix)
    {
        return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
    }

    /*这个好像是用来检测DD dump的*/
    bool isMemoryReadable(uintptr_t startAddress, size_t size)
    {
        // Check if memory is readable
        for (size_t i = 0; i < size; i += sysconf(_SC_PAGESIZE))
        {
            int result = access(reinterpret_cast<const char *>(startAddress + i), R_OK);
            if (result != 0)
            {
                return false;
            }
        }
        return true;
    }

    bool isQualifiedMemoryPattern(const std::string &permissions, const std::string &inPermission)
    {
        return permissions.find(inPermission.c_str()) != std::string::npos;
    }

    void searchMemory(const std::string &targetLibrary, const std::string &dumpDir)
    {
        sleep(30);
        LOGI("开始在maps中查找 %s 的模式", targetLibrary.c_str());
        std::ifstream mapsFile("/proc/self/maps");
        if (!mapsFile.is_open())
        {
            LOGI("无法打开 /proc/self/maps");
            return;
        }
        std::string line;
        bool foundAny = false;
        while (std::getline(mapsFile, line))
        {
            std::istringstream iss(line);
            std::string addressRange, permissions, offset, device, inode, pathname;
            if (!(iss >> addressRange >> permissions >> offset >> device >> inode && std::getline(iss, pathname)))
            {
                continue; // 解析失败，跳过此行
            }
            pathname.erase(0, pathname.find_first_not_of(' '));
            if (pathname.find(targetLibrary) != std::string::npos)
            {
                uintptr_t startAddress, endAddress;
                sscanf(addressRange.c_str(), "%lx-%lx", &startAddress, &endAddress);
                size_t size = endAddress - startAddress;
                LOGI("在地址范围 %lx-%lx 找到 %s (%zu 字节)", startAddress, endAddress, targetLibrary.c_str(), size);
                if (isQualifiedMemoryPattern(permissions, "r"))
                {
                    std::stringstream ss;
                    ss << "libil2cpp_dump_" << std::hex << startAddress << "_" << std::hex << endAddress << ".bin";
                    std::string filename = ss.str();
                    dumpMemory((void *)(startAddress), size, filename, dumpDir);
                    foundAny = true;
                }
                else
                {
                    LOGI("地址范围 %lx-%lx 的内存不可读", startAddress, endAddress);
                }
            }
        }
        if (!foundAny)
        {
            LOGI("未找到 %s 的任何内存段", targetLibrary.c_str());
        }
        if (mapsFile.eof())
        {
            LOGI("已经到达 /proc/self/maps 的末尾，结束搜索。");
        }
        else if (mapsFile.fail())
        {
            LOGI("读取 /proc/self/maps 时发生错误。");
        }
        mapsFile.close();
        return;
    }

    void searchMemory(const std::vector<uint8_t> &bytesToSearch, const std::vector<std::string> &disabledPathsPrefix, const std::string &inPermission, std::function<void(uintptr_t, const std::string &, bool *)> callback, bool isLog)
    {
        bool isContinue = true;
        std::ifstream mapsFile("/proc/self/maps");
        if (!mapsFile.is_open())
        {
            LOGI("Failed to open /proc/self/maps");
            return;
        }
        std::string line;
        while (isContinue)
        {
            if (!std::getline(mapsFile, line))
            {
                LOGI("End of File has reached, restart to search");
                mapsFile.clear();
                mapsFile.seekg(0, std::ios::beg);
                continue;
            }
            std::istringstream iss(line);
            std::string addressRange, permissions, offset, device, inode, pathname;
            if (!(iss >> addressRange >> permissions >> offset >> device >> inode && std::getline(iss, pathname)))
            {
                continue; // Parsing failed, skip this line
            }
            pathname.erase(0, pathname.find_first_not_of(' '));
            bool isDisabled = std::find_if(disabledPathsPrefix.begin(), disabledPathsPrefix.end(), [&pathname](const std::string &prefix)
                                           { return pathname.find(prefix) == 0; }) != disabledPathsPrefix.end();
            if (isDisabled)
            {
                continue; // Skip disabled paths
            }
            if (!isQualifiedMemoryPattern(permissions, inPermission))
            {
                continue;
            }
            uintptr_t startAddress, endAddress;
            sscanf(addressRange.c_str(), "%lx-%lx", &startAddress, &endAddress);
            size_t pageSize = endAddress - startAddress;
            if (isLog == true)
            {
                LOGI("<Scanning Memory> %lx-%lx %lx %s %s", startAddress, endAddress, pageSize, permissions.c_str(), pathname.c_str());
            }
            for (size_t i = 0; i <= pageSize - bytesToSearch.size(); i += 1)
            {
                uint8_t *ptr = reinterpret_cast<uint8_t *>(startAddress + i);
                if (memcmp(ptr, bytesToSearch.data(), bytesToSearch.size()) == 0)
                {
                    std::stringstream concatenatedBytes;
                    for (size_t j = 0; j < bytesToSearch.size(); ++j)
                    {
                        concatenatedBytes << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytesToSearch[j]) << " ";
                    }
                    std::string concatenatedBytesStr = concatenatedBytes.str();
                    concatenatedBytesStr.pop_back();
                    // LOGI("Found bytes at address %p: %s %s", ptr, concatenatedBytesStr.c_str(), line.c_str());
                    callback(startAddress + i, line, &isContinue);
                }
            }
        }
        LOGI("Searching Memory Done");
        return;
    }

    void searchMemory(const std::string &stringToSearch, const std::vector<std::string> &disabledPathsPrefix, const std::string &inPermission, std::function<void(uintptr_t, const std::string &, bool *)> callback, bool isLog)
    {
        bool isContinue = true;
        std::ifstream mapsFile("/proc/self/maps");
        if (!mapsFile.is_open())
        {
            LOGI("Failed to open /proc/self/maps");
            return;
        }
        std::string line;
        while (isContinue)
        {
            if (!std::getline(mapsFile, line))
            {
                LOGI("End of File has reached, restart to search");
                mapsFile.clear();
                mapsFile.seekg(0, std::ios::beg);
                continue;
            }
            std::istringstream iss(line);
            std::string addressRange, permissions, offset, device, inode, pathname;
            if (!(iss >> addressRange >> permissions >> offset >> device >> inode && std::getline(iss, pathname)))
            {
                continue; // Parsing failed, skip this line
            }
            pathname.erase(0, pathname.find_first_not_of(' '));
            bool isDisabled = std::find_if(disabledPathsPrefix.begin(), disabledPathsPrefix.end(), [&pathname](const std::string &prefix)
                                           { return pathname.find(prefix) == 0; }) != disabledPathsPrefix.end();
            if (isDisabled)
            {
                continue; // Skip disabled paths
            }
            if (!isQualifiedMemoryPattern(permissions, inPermission))
            {
                continue;
            }
            uintptr_t startAddress, endAddress;
            sscanf(addressRange.c_str(), "%lx-%lx", &startAddress, &endAddress);
            size_t pageSize = endAddress - startAddress;
            if (isLog == true)
            {
                LOGI("<Scanning Memory> %lx-%lx %lx %s %s", startAddress, endAddress, pageSize, permissions.c_str(), pathname.c_str());
            }
            const char *searchStr = stringToSearch.c_str();
            size_t searchLen = stringToSearch.size();
            for (size_t i = 0; i <= pageSize - searchLen; i += 1)
            {
                const char *ptr = reinterpret_cast<const char *>(startAddress + i);
                if (memcmp(ptr, searchStr, searchLen) == 0)
                {
                    callback(startAddress + i, line, &isContinue);
                }
            }
        }
        LOGI("Searching Memory Done");
        return;
    }

    bool containsByteSequence(uint64_t address, const std::vector<unsigned char> &sequence)
    {
        // Read bytes from memory starting at the specified address
        std::vector<unsigned char> bytes;
        for (size_t i = 0; i < sequence.size(); ++i)
        {
            // Read byte from memory at each address offset
            unsigned char byte = *reinterpret_cast<unsigned char *>(address + i);
            bytes.push_back(byte);
        }
        // Check if the bytes contain the target sequence
        for (size_t i = 0; i <= bytes.size() - sequence.size(); ++i)
        {
            bool found = true;
            for (size_t j = 0; j < sequence.size(); ++j)
            {
                if (bytes[i + j] != sequence[j])
                {
                    found = false;
                    break;
                }
            }
            if (found)
                return true;
        }
        return false;
    }

    void hexdump(size_t ptr, int buflen)
    {
        unsigned char *buf = (unsigned char *)ptr;
        int i, j;
        char tmpBuf[2000] = {0};
        LOGI("hexdump %p:", ptr);
        for (i = 0; i < buflen; i += 16)
        {
            char *p = tmpBuf;
            p += sprintf(p, "%06x: ", i);
            for (j = 0; j < 16; j++)
                if (i + j < buflen)
                    p += sprintf(p, "%02x ", buf[i + j]);
                else
                    p += sprintf(p, "   ");
            p += sprintf(p, " ");
            for (j = 0; j < 16; j++)
                if (i + j < buflen)
                    p += sprintf(p, "%c", isprint(buf[i + j]) ? buf[i + j] : '.');
            LOGI("%s", tmpBuf);
        }
    }

    Dl_info getAddrDLInfo(void *addr)
    {
        Dl_info addrInfo = {0};
        dladdr(addr, &addrInfo);
        addrInfo.dli_fname =
            addrInfo.dli_fname == 0 ? "null" : addrInfo.dli_fname;
        addrInfo.dli_sname =
            addrInfo.dli_sname == 0 ? "null" : addrInfo.dli_sname;
        return addrInfo;
    }

    void getProcessName(char *processName)
    {
        FILE *fp = fopen("/proc/self/cmdline", "r");
        fread(processName, 0x100, 0x1, fp);
        return;
    }

    void createDirectories(std::string &path)
    {
        mode_t mode = 0777;
        size_t pre = 0, pos;
        std::string dir;
        if (path[path.size() - 1] != '/')
        {
            // 如果路径不是以 '/' 结尾，加上 '/'
            path += '/';
        }
        while ((pos = path.find_first_of('/', pre)) != std::string::npos)
        {
            dir = path.substr(0, pos++);
            pre = pos;
            if (dir.size() == 0)
                continue; // 忽略第一个 '/'
            // 创建目录，如果目录已经存在则忽略错误
            if (mkdir(dir.c_str(), mode) && errno != EEXIST)
            {
                LOGI("Error creating directory %s %s", dir.c_str(), strerror(errno));
                break;
            }
        }
        return;
    }

    void dumpMemory(void *ptr, size_t size, const std::string &filename, const std::string &folder)
    {
        FILE *fp;
        std::string filepath = folder + "/" + filename; // 构建文件路径
        // 提取文件路径（目录）
        std::string::size_type pos = filepath.find_last_of("/\\");
        std::string dir = (pos == std::string::npos) ? "" : filepath.substr(0, pos);
        // 检查文件夹是否存在，不存在则创建
        struct stat st;
        if (stat(dir.c_str(), &st) != 0)
        {
            createDirectories(dir); // 创建文件夹
        }
        // 打开文件，以二进制写入方式
        fp = fopen(filepath.c_str(), "wb");
        if (fp == NULL)
        {
            LOGI("无法打开文件 %s", filepath.c_str());
            return;
        }
        LOGI("Dumping Memory");
        // 写入内存块内容到文件
        fwrite(ptr, 1, size, fp);
        // 关闭文件
        fclose(fp);
        LOGI("成功将内存块内容写入文件 %s", filepath.c_str());
        return;
    }

    void signalHandler(int signum, siginfo_t *info, void *context)
    {
        ucontext_t *uc = (ucontext_t *)context;
        LOGI("Received signal %d\n", signum);
#if defined(__arm__)
        LOGI("R0: 0x%x\n", uc->uc_mcontext.arm_r0);
        LOGI("R1: 0x%x\n", uc->uc_mcontext.arm_r1);
        LOGI("R2: 0x%x\n", uc->uc_mcontext.arm_r2);
        LOGI("R3: 0x%x\n", uc->uc_mcontext.arm_r3);
        LOGI("R4: 0x%x\n", uc->uc_mcontext.arm_r4);
        LOGI("R5: 0x%x\n", uc->uc_mcontext.arm_r5);
        LOGI("R6: 0x%x\n", uc->uc_mcontext.arm_r6);
        LOGI("R7: 0x%x\n", uc->uc_mcontext.arm_r7);
        LOGI("R8: 0x%x\n", uc->uc_mcontext.arm_r8);
        LOGI("R9: 0x%x\n", uc->uc_mcontext.arm_r9);
        LOGI("R10: 0x%x\n", uc->uc_mcontext.arm_r10);
        LOGI("FP: 0x%x\n", uc->uc_mcontext.arm_fp);
        LOGI("IP: 0x%x\n", uc->uc_mcontext.arm_ip);
        LOGI("SP: 0x%x\n", uc->uc_mcontext.arm_sp);
        LOGI("LR: 0x%x\n", uc->uc_mcontext.arm_lr);
        LOGI("PC: 0x%x\n", uc->uc_mcontext.arm_pc);
        LOGI("CPSR: 0x%x\n", uc->uc_mcontext.arm_cpsr);
#elif defined(__aarch64__)
        for (int i = 0; i < 31; i++)
        {
            LOGI("X%d: 0x%llx\n", i, uc->uc_mcontext.regs[i]);
        }
        LOGI("SP: 0x%llx\n", uc->uc_mcontext.sp);
        LOGI("PC: 0x%llx\n", uc->uc_mcontext.pc);
        LOGI("PSTATE: 0x%llx\n", uc->uc_mcontext.pstate);
#else
        LOGI("Unsupported architecture\n");
#endif
        exit(EXIT_FAILURE);
        return;
    }

    // 设置信号处理程序
    void setupSignalHandler(size_t sigNo)
    {
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = signalHandler;
        sigemptyset(&sa.sa_mask);
        if (sigaction(sigNo, &sa, NULL) == -1)
        {
            LOGE("sigaction");
            exit(EXIT_FAILURE);
        }
        return;
    }
}