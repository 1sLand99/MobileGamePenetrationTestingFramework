#include <dobby.h>
#include <jni.h>
#include <string>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <thread>
#include <sys/time.h>
#include <byopen.h>
#include <Log.h>

extern "C" {
    bool startHacking(JavaVM* vm);
}

std::string soName = "libc.so";

static unsigned long long startTime = 0;
static unsigned long long currentTime = 0;
static unsigned long long deltaTime = 0;
static unsigned long long prevTime = 0;
static int (*gettimeofday_f)(struct timeval*tv, struct timezone *tz);
static int gettimeofday_hook(struct timeval*tv, struct timezone *tz)
{
    LOGI("gettimeofday_hook 1");
    int ret = gettimeofday_f(tv, tz);
    if(startTime == 0)
    {
        startTime = (unsigned long long)(tv->tv_sec * 1000000LL + tv->tv_usec);
        prevTime = startTime;
        return ret;
    }
    currentTime = (unsigned long long)(tv->tv_sec * 1000000LL + tv->tv_usec);
    deltaTime = currentTime - prevTime;
    prevTime = currentTime;
    startTime += (unsigned long long)(deltaTime * MySpeed);
    tv->tv_sec = (__kernel_time_t)(startTime / 1000000LL);
    tv->tv_usec = (__kernel_suseconds_t)(startTime % 1000000LL);
    LOGI("gettimeofday_hook 2");
    return ret;
}

static unsigned long long startTime1 = 0;
static unsigned long long currentTime1 = 0;
static unsigned long long deltaTime1 = 0;
static unsigned long long prevTime1 = 0;
static int (*clock_gettime_f)(clockid_t clk_id,struct timespec *tp);
static int clock_gettime_hook(clockid_t clk_id,struct timespec *tp)
{
    LOGI("clock_gettime_hook 1");
    if(clk_id != 1)
    {
        return clock_gettime_f(clk_id, tp);
    }
    if(startTime1 == 0)
    {
        startTime1 = (unsigned long long)(tp->tv_sec * 1000000000LL + tp->tv_nsec);
        prevTime1 = startTime1;
        return clock_gettime_f(clk_id, tp);
    }
    currentTime1 = (unsigned long long)(tp->tv_sec * 1000000000LL + tp->tv_nsec);
    deltaTime1 = currentTime1 - prevTime1;
    prevTime1 = currentTime1;
    startTime1 += (unsigned long long)(deltaTime1 * MySpeed);
    tp->tv_sec = (__kernel_time_t)(startTime1 / 1000000000LL);
    tp->tv_nsec = (long)(startTime1 % 1000000000LL);
    LOGI("clock_gettime_hook 2");
    return clock_gettime_f(clk_id, tp);
}

static void myCheckMap(void)
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
            LOGI("isEnd hit");
        }
        sscanf(line, "%zx-%zx %s %zx %s %zx %s", &base_addr, &end_addr, permission, &offset, fd, &softLink, path);
        if (strstr(path, soName.c_str()))
        {
            LOGI("match so name");
            if (offset == 0)
            {
                LOGI("match offset");
                if(strcasestr(permission, "r-xp"))
                {
                    LOGI("match permission");
                    LOGI("Library is Found! %s", path);
                    fclose(fp);
                    break;
                }
            }
        }
    }
    by_pointer_t soHandle = by_dlopen(soName.c_str(), BY_RTLD_LAZY);
    LOGI("soHandle : %p", soHandle);
    LOGI("wait 10s to start speed hacking");
    sleep(10);
    LOGI("wait 10s finish");
    if(soHandle)
    {
        by_pointer_t gettimeofdayAddr = by_dlsym(soHandle, "gettimeofday");
        DobbyHook(gettimeofdayAddr, (dobby_dummy_func_t)gettimeofday_hook, (dobby_dummy_func_t*)&gettimeofday_f);
        //by_pointer_t clock_gettimeAddr = by_dlsym(soHandle, "clock_gettime");
        //DobbyHook(clock_gettimeAddr, (dobby_dummy_func_t)clock_gettime_hook, (dobby_dummy_func_t*)&clock_gettime_f);
        by_dlclose(soHandle);
    }
}

bool startHacking(JavaVM* vm) {
    LOGI("fucking start now");
    pthread_t mythread;
    pthread_create(&mythread, nullptr, reinterpret_cast<void *(*)(void *)>(myCheckMap), nullptr);
    return true;
}