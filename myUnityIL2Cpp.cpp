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

extern "C"
{
    bool startHacking(JavaVM *vm);
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
    LOGI("enter loop");
    FILE *fp = fopen(fileName, "r");
    while (1)
    {
        char *isEnd = fgets(line, 512, fp);
        if (isEnd == nullptr)
        {
            fseek(fp, 0, 0);
        }
        sscanf(line, "%zx-%zx %s %zx %s %zx %s", &base_addr, &end_addr, permission, &offset, fd, &softLink, path);
        if (strstr(path, il2cppSoName))
        {
            if (offset == 0)
            {
                if(strcasestr(permission, "r-xp")) // 正常來說用這個，但加密後的il2cpp 需要換成下面這個
                //if(strcasestr(permission, "rwxp"))
                {
                    LOGI("match permission, base:%p", base_addr);
                    LOGI("Library is Found! %s", path);
                    fclose(fp);
                    break;
                }
            }
        }
    }
    by_pointer_t bySoHandle = by_dlopen(il2cppSoName, BY_RTLD_LAZY);
    /*Dump*/
    sleep(5);
    std::string outDir = "/data/data/";
    char packageName[255] = "com.xt.xt.xt";
    // char processName[255] = {0};
    // getProcessName(processName);
    outDir += packageName;
    //il2cpp_basedump(base_addr, (char*)outDir.c_str()); 这个好像是特别强的对抗才用的。
    il2cpp_dump((void*)bySoHandle, (char*)outDir.c_str());
    /*Speeding*/
    //il2cpp_speed(bySoHandle);
    /*Tracing*/
    // il2cpp_filter(bySoHandle);
    LOGI("Done find");
    return;
}

bool startHacking(JavaVM *vm)
{
    LOGI("Unity Il2Cpp hack now");
    pthread_t mythread;
    pthread_create(&mythread, nullptr, reinterpret_cast<void *(*)(void *)>(myCheckMap), nullptr);
    return true;
}