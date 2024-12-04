#include <jni.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <thread>
#include <link.h>
#include <sys/stat.h>
#include <byopen.h>
#include <base64.h>
#include <dobby.h>
#include <Log.h>
#include <GlobalInfo.h>
#include <il2cpp_dump.h>
#include <PrintStack.h>
#include <fcntl.h>
#include <Util.h>
#include <HwBreakpointManager.h>
#include <NativeSandBox.h>

extern "C"
{
    bool startHacking(JavaVM *vm);
}

static size_t readAddr = 0;

int (*oldopen)(const char *pathname, int flags, mode_t mode);
int newopen(const char *pathname, int flags, mode_t mode)
{
    return oldopen(pathname, flags, mode);
}

void * (*oldInvokableCall)(void * thiz, void * methodInfo, void * a2, void * a3, void * a4);
void * newInvokableCall(void * thiz, void * methodInfo, void * a2, void * a3, void * a4)
{
    LOGI("newInvokableCall enter");
    dumpStackUnwind();
    return oldInvokableCall(thiz, methodInfo, a2, a3, a4);
}

void myCheckMap(void)
{
    LOGI("checking Map");
    size_t pid = getpid();
    LOGI("pid : %zu", pid);
    char line[512] = {0};
    char fileName[512] = {0};
    sprintf(fileName, "/proc/%zu/maps", pid);
    LOGI("fileName : %s", fileName);
    size_t base_addr;
    size_t end_addr;
    char permission[512] = {0};
    size_t offset = 0;
    char fd[512] = {0};
    size_t softLink = 0;
    char path[512] = {0};
    LOGI("enter loop 666");
    FILE *fp = fopen(fileName, "r");
    while (1)
    {
        char *isEnd = fgets(line, 512, fp);
        if (isEnd == nullptr)
        {
            fseek(fp, 0, 0);
            LOGI("isEnd hit");
        }
        sscanf(line, "%zx-%zx %s %zx %s %zx %s", &base_addr, &end_addr, permission, &offset, fd, &softLink, path);
        if (strstr(path, testSoName))
        {
            LOGI("match so name");
            if (offset == 0)
            {
                LOGI("match offset");
                if(strcasestr(permission, "r-xp"))
                {
                    LOGI("match permission");
                    LOGI("Library is Found! %s", path);
                    readAddr = base_addr + 0x4a490;
                    LOGI("readAddr:%zx", readAddr);
                    initJmpTable();
                    initHardwareModule();
                    addHardwareBreakpoint(readAddr, HW_BREAKPOINT_LEN_8, HW_BREAKPOINT_RW,
                    0, 0, 0, 0, 
                    0, 0, 0, HW_BREAKPOINT_RW, 0);
                    fclose(fp);
                    break;
                }
            }
        }
    }
    LOGI("Done find");
    return;
}

bool startHacking(JavaVM *vm)
{
    LOGI("fucking start now");
    pthread_t mythread;
    pthread_create(&mythread, nullptr, reinterpret_cast<void *(*)(void *)>(myCheckMap), nullptr);
    return true;
}