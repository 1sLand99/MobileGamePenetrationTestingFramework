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
#include <Util.h>
#include <Log.h>
#include <GlobalInfo.h>
#include <PrintStack.h>
#include <fcntl.h>
#include <LuaHeaders/lua535/lua.h>

extern "C"
{
    bool startHacking(JavaVM *vm);
}

static bool isInject = false;
static bool isRegisterCFunction = false;
static bool isGTabSet = false;

static unsigned long long startTime = 0;
static unsigned long long currentTime = 0;
static unsigned long long deltaTime = 0;
static unsigned long long prevTime = 0;
static int (*gettimeofday_f)(struct timeval*tv, struct timezone *tz);
static int gettimeofday_hook(struct timeval*tv, struct timezone *tz)
{
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
    return ret;
}

static unsigned long long startTime1 = 0;
static unsigned long long currentTime1 = 0;
static unsigned long long deltaTime1 = 0;
static unsigned long long prevTime1 = 0;
static int (*clock_gettime_f)(clockid_t clk_id,struct timespec *tp);
static int clock_gettime_hook(clockid_t clk_id,struct timespec *tp)
{
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
    return clock_gettime_f(clk_id, tp);
}

int (*oldopen)(const char *pathname, int flags, mode_t mode);
int newopen(const char *pathname, int flags, mode_t mode)
{
    if (strstr(pathname, "FightPanel.lua") && !strstr(pathname, ".luac"))
    {
        LOGI("newopen:%s", pathname);
        return oldopen("/data/local/tmp/FightPanel.lua", flags, mode);
    }
    // dumpStackUnwind();
    return oldopen(pathname, flags, mode);
}

const char *(*oldlua_tolstring)(void *L, int index, size_t *len);

int (*oldluaL_loadbuffer)(void *lua_State, const char *buff, size_t size, const char *scriptName, void *version);

int (*oldluaL_loadfilex)(void *L, const char *filename, const char *mode);

int (*oldluaL_loadstring)(void *L, const char *string);

void injectLuaScript(void *lua_State, const char *buff, size_t size, const char *scriptName,
                     void *version);

void traceLuaEngine(void *lua_State, const char *buff, size_t size, const char *scriptName, void *version,
                    bool *isInject);

void dumpLuaScript(void *lua_State, const char *buff, size_t size, const char *scriptName,
                   void *version);

void replaceLuaScript(void *lua_State, const char **buff, size_t *size, const char *scriptName,
                      void *version);

void registerCFunction(void *lua_State, const char *scriptName);

void setGTab(void *lua_State, const char *scriptName);

int MyLog(void *lua_State)
{
    if (lua_tolstring)
    {
        const char *MyLog = lua_tolstring(lua_State, 1, 0);
        LOGI("MyLog: %s", MyLog);
    }
    return 0;
}

void invokeLuaMethod(void *lua_State)
{
    LOGI("invokeLuaMethod 0 !!!");
    lua_getglobal(lua_State, "MyLog");
    lua_pushstring(lua_State, "Call From Hacker!");
    lua_pcall(lua_State, 1, 0, 0);
    LOGI("invokeLuaMethod 1 !!!");
    return;
}

void setHookCallBack(void *lua_State, lua_Debug *ar)
{
    if (lua_getinfo(lua_State, "nSltufL", ar) && ar->source && ar->name && ar->currentline != -1)
    {
        if (ar->source && ar->name)
        {
            if (strcasestr(ar->source, ""))
            {
                LOGI("source:[%s] name:[%s] what:[%s] namewhat:[%s] nups:[%d] nparams:[%d]", ar->source,
                     ar->name, ar->what, ar->namewhat, ar->nups, ar->nparams);
                invokeLuaMethod(lua_State);
                // 那么下一个问题，就是，怎么才能够从C层访问 upvalue 的数据?
            }
        }
    }
    return;
}

void registerCFunction(void *lua_State, const char *scriptName)
{
    if (!isRegisterCFunction && !strcmp(scriptName, registerCFuncLuaScriptName))
    {
        // 注册C MyLog 函数
        LOGI("registerCFunction enter");
        isRegisterCFunction = true;
        /*****************************************************************************************************************************************/
        if(lua_settop != 0 && lua_pushcclosure != 0 && lua_setglobal != 0)
        {
            //LOGI("lua_settop != 0 && lua_pushcclosure != 0 && lua_setglobal != 0");
            // 如果找的到 setglobal的话, 使用这里的逻辑
            lua_settop(lua_State, 0);
            lua_pushcclosure(lua_State, (void *)MyLog, 0);
            lua_setglobal(lua_State, "MyLog");
        }
        else if(lua_pushcclosure != 0 && lua_setfield != 0)
        {
            //LOGI("lua_pushcclosure != 0 && lua_setfield != 0");
            // 如果找不到 setglobal的话，使用这里的逻辑
            lua_pushcclosure(lua_State, (void *)MyLog, 0);
            lua_setfield(lua_State, LUA_GLOBALSINDEX, "MyLog");
        }
        else
        {
            LOGI("registerCFunction fails");
        }
        LOGI("registerCFunction finish");
        /*****************************************************************************************************************************************/
    }
    return;
}

void setGTab(void *lua_State, const char *scriptName)
{
    if (!isGTabSet && !strcmp(scriptName, setGTabLuaScriptName))
    {
        LOGI("setGTab enter");
        isGTabSet = true;
        const char *bufSetGTable =
            "setmetatable(_G, {\n"
            "    __newindex = function(table, key, value)\n"
            "        rawset(table, key, value)\n"
            "    end\n"
            "})";
        size_t len = strlen(bufSetGTable);
        oldluaL_loadbuffer(lua_State, bufSetGTable, len, "setGTable.lua", NULL);
        if (lua_pcall(lua_State, 0, 0, 0))
        {
            const char *errorMsg = lua_tolstring(lua_State, -1, 0);
            LOGI("errorMsg : %s", errorMsg);
            lua_settop(lua_State, -2);
        }
        LOGI("setGTab leave");
    }
    return;
}

int newluaL_loadstring(void *L, const char *string)
{
    LOGI("loadstring:%s", string);
    dumpStackUnwind();
    return oldluaL_loadstring(L, string);
}

const char *newlua_tolstring(void *L, int index, size_t *len)
{
    const char *ret = oldlua_tolstring(L, index, len);
    if (ret != nullptr)
    {
        LOGI("lua_tolstring:%s", ret);
    }
    return ret;
}

int newluaL_loadfilex(void *L, const char *filename, const char *mode)
{
    LOGI("filename:%s", filename);
#ifdef logLua
    if (filename != nullptr)
    {
        LOGI("filename:%s", filename);
    }
#endif
    return oldluaL_loadfilex(L, filename, mode);
}

void logLuaScript(const char * scriptName, size_t size)
{
    if(scriptName != nullptr)
    {
        LOGI("scriptName:%s size:%zu", scriptName, size);
    }
    return;
}

int newluaL_loadbuffer(void *lua_State, const char *buff, size_t size, const char *scriptName,
                       void *version)
{
#ifdef registerLua
    setGTab(lua_State, scriptName);
    registerCFunction(lua_State, scriptName);
#endif

#ifdef logLua
    logLuaScript(scriptName, size);
#endif

#ifdef replaceLua
    replaceLuaScript(lua_State, &buff, &size, scriptName, version);
#endif

#ifdef dumpLua
    dumpLuaScript(lua_State, buff, size, scriptName, version);
#endif

#ifdef injectLua
    injectLuaScript(lua_State, buff, size, scriptName, version);
#endif

#ifdef traceLua
    traceLuaEngine(lua_State, buff, size, scriptName, version, &isInject);
#endif

    return oldluaL_loadbuffer(lua_State, buff, size, scriptName, version);
}

void replaceLuaScript(void *lua_State, const char **buff, size_t *size, const char *scriptName,
                      void *version)
{
    if (scriptName != 0 && strcmp(scriptName, "") && strlen(scriptName) <= 0x50 &&
        !strcasecmp(scriptName, replaceLuaScriptName))
    {
        LOGI("replace scriptName:%s", scriptName);
        unsigned char *decodeScript = base64_decode((unsigned char *)replaceLuaScriptBuff);
        *size = strlen(reinterpret_cast<const char *const>(decodeScript));
        *buff = reinterpret_cast<const char *>(decodeScript);
    }
    return;
}

void dumpLuaScript(void *lua_State, const char *buff, size_t size, const char *scriptName,
                   void *version)
{
    static size_t scriptNameCount = 0;
    if (scriptName != nullptr && strlen(scriptName) <= 0x50 && size >= 0x30)
    //if (scriptName != nullptr && size >= 0x50)
    {
        std::string folderName = "/data/data/";
        // packagename 我需要动态获取才行.
        char processName[0x100] = {0};
        getProcessName(processName);
        folderName += processName;
        folderName += "/lua/";
        folderName += dirname(scriptName);
        folderName += "/";
        std::string stdMkdirCmd = "mkdir -p ";
        stdMkdirCmd = stdMkdirCmd + folderName.c_str();
        std::string fileName = basename(scriptName);
        if(strlen(scriptName) == 0)
        {
            LOGI("strlen == 0");
            fileName = std::to_string(scriptNameCount);
            scriptNameCount++;
        }
        std::string fullName = folderName + fileName + ".lua";
        LOGI("file:[%s] full:[%s] sz:[%zx]", fileName.c_str(), fullName.c_str(), size);
        system(stdMkdirCmd.c_str());
        //LOGI("mkdir success");
        FILE *fp = fopen(fullName.c_str(), "wb+");
        //LOGI("fopen success");
        fwrite(buff, size, 1, fp);
        //LOGI("fwrite done");
        fclose(fp);
        //LOGI("fclose done");
    }
    return;
}

void traceLuaEngine(void *lua_State, const char *buff, size_t size, const char *scriptName, void *version,
                    bool *isInject)
{
    if (!*isInject && !strcmp(scriptName, injectLuaScriptName))
    {
        *isInject = true;
        LOGI("set hook script name : %s", scriptName);
        /*参数 count 只在掩码中包含有 LUA_MASKCOUNT 才有意义。*/
        /*http://www.jishuchi.com/read/lua-5.3/1982*/
        // lua_sethook(lua_State, setHookCallBack, LUA_MASKCALL | LUA_MASKRET, -1);
        lua_sethook(lua_State, setHookCallBack, LUA_MASKCALL, -1);
        LOGI("set hook end");
    }
    return;
}

void injectLuaScript(void *lua_State, const char *buff, size_t size, const char *scriptName,
                     void *version)
{
    if (!isInject && !strcmp(scriptName, injectLuaScriptName))
    {
        LOGI("inject scriptName : %s", scriptName);
        isInject = true;
        unsigned char *decodeScript = base64_decode((unsigned char *)injectLuaScriptBuff);
        size_t len = strlen(reinterpret_cast<const char *const>(decodeScript));
        oldluaL_loadbuffer(lua_State, reinterpret_cast<const char *>(decodeScript), len, "inject.lua", 0);
        if (lua_pcall(lua_State, 0, 0, 0))
        {
            const char *errorMsg = lua_tolstring(lua_State, -1, 0);
            LOGI("errorMsg : %s", errorMsg);
            lua_settop(lua_State, -2);
        }
        LOGI("inject scripting end");
    }
    return;
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
        if (strstr(path, luaSoName))
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
    by_pointer_t bySoHandle = by_dlopen(luaSoName, BY_RTLD_LAZY);
    if(bySoHandle != nullptr)
    {
        lua_getinfo = (lua_getinfoFuncType)by_dlsym(bySoHandle, "lua_getinfo");
        if (lua_getinfo != 0)
        {
            LOGI("lua_getinfo:%p", lua_getinfo);
        }
        lua_sethook = (lua_sethookFuncType)by_dlsym(bySoHandle, "lua_sethook");
        if (lua_sethook != 0)
        {
            LOGI("lua_sethook:%p", lua_sethook);
        }
        lua_setglobal = (lua_setglobalFuncType)by_dlsym(bySoHandle, "lua_setglobal");
        if (lua_setglobal != 0)
        {
            LOGI("lua_setglobal:%p", lua_setglobal);
        }
        lua_settop = (lua_settopFuncType)by_dlsym(bySoHandle, "lua_settop");
        if (lua_settop != 0)
        {
            LOGI("lua_settop:%p", lua_settop);
        }
        lua_tolstring = (lua_tolstringFuncType)by_dlsym(bySoHandle, "lua_tolstring");
        if (lua_tolstring != 0)
        {
            LOGI("lua_tolstring:%p", lua_tolstring);
        }
        lua_pushcclosure = (lua_pushcclosureFuncType)by_dlsym(bySoHandle, "lua_pushcclosure");
        if (lua_pushcclosure != 0)
        {
            LOGI("lua_pushcclosure:%p", lua_pushcclosure);
        }
        lua_setfield = (lua_setfieldFuncType)by_dlsym(bySoHandle, "lua_setfield");
        if (lua_setfield != 0)
        {
            LOGI("lua_setfield:%p", lua_setfield);
        }
        lua_pcall = (lua_pcallFuncType)by_dlsym(bySoHandle, "lua_pcall");
        if (lua_pcall != 0)
        {
            LOGI("lua_pcall:%p", lua_pcall);
        }
        lua_getglobal = (lua_getglobalFuncType)by_dlsym(bySoHandle, "lua_getglobal");
        if (lua_getglobal != 0)
        {
            LOGI("lua_getglobal:%p", lua_getglobal);
        }
        lua_pushstring = (lua_pushstringFuncType)by_dlsym(bySoHandle, "lua_pushstring");
        if (lua_pushstring != 0)
        {
            LOGI("lua_pushstring:%p", lua_pushstring);
        }
        lua_getupvalue = (lua_getupvalueFuncType)by_dlsym(bySoHandle, "lua_getupvalue");
        if (lua_getupvalue != 0)
        {
            LOGI("lua_getupvalue:%p", lua_getupvalue);
        }
        /*
        luaL_loadbuffer  
        luaL_loadbufferx 
        xluaL_loadbuffe  */
        void *luaL_loadbufferxAddr = (void *)by_dlsym(bySoHandle, "luaL_loadbufferx");//luaL_loadbufferx 多了一个 version 结尾参数, luaL_loadbuffer 正常
        if (luaL_loadbufferxAddr != 0)
        {
            LOGI("luaL_loadbufferxAddr : 1 %p 0x%zx", luaL_loadbufferxAddr, *(size_t *)luaL_loadbufferxAddr);
            DobbyHook((void *)luaL_loadbufferxAddr, (dobby_dummy_func_t)newluaL_loadbuffer, (dobby_dummy_func_t *)&oldluaL_loadbuffer);
            LOGI("newluaL_loadbuffer:[%p] oldluaL_loadbuffer:[%p]", newluaL_loadbuffer, oldluaL_loadbuffer);
        }
    }

    // void* luaL_loadfilex_addr = (void*)by_dlsym(bySoHandle, "luaL_loadfilex");
    // if(luaL_loadfilex_addr != 0)
    // {
    //     LOGI("luaL_loadfilex_addr : 1 %p 0x%x", luaL_loadfilex_addr, *(size_t *)luaL_loadfilex_addr);
    //     DobbyHook((void *)luaL_loadfilex_addr, (dobby_dummy_func_t)newluaL_loadfilex, (dobby_dummy_func_t *)&oldluaL_loadfilex);
    //     LOGI("newluaL_loadfilex:[%p] oldluaL_loadfilex:[%p]", newluaL_loadfilex, oldluaL_loadfilex);
    // }

    // int luaL_loadstring (lua_State *L, const char *s);
    // void* luaL_loadstring_addr = (void*)by_dlsym(bySoHandle, "luaL_loadstring");
    // if(luaL_loadstring_addr != 0)
    // {
    //     LOGI("luaL_loadstring_addr : 1 %p 0x%x", luaL_loadstring_addr, *(size_t *)luaL_loadstring_addr);
    //     DobbyHook((void *)luaL_loadstring_addr, (dobby_dummy_func_t)newluaL_loadstring, (dobby_dummy_func_t *)&oldluaL_loadstring);
    //     LOGI("newluaL_loadstring:[%p] oldluaL_loadstring:[%p]", newluaL_loadstring, oldluaL_loadstring);
    // }

    // //void * lual_tolstringAddr = DobbySymbolResolver("libcocos2dlua.so", "lua_tolstring");
    // void * lual_tolstringAddr = by_dlsym(bySoHandle, "lua_tolstring");
    // if(lual_tolstringAddr != 0)
    // {
    //     LOGI("lual_tolstringAddr:[%p] [0x%x] [0x%x]", lual_tolstringAddr, *(size_t *)lual_tolstringAddr, oldlua_tolstring);
    //     //DobbyHook(lual_tolstringAddr, (dobby_dummy_func_t)newlua_tolstring, (dobby_dummy_func_t*)&oldlua_tolstring);
    // #ifdef __aarch64__
    //     // A64HookFunction((void*)lual_tolstringAddr, (void*)newlua_tolstring, (void**)&oldlua_tolstring);
    //     // LOGI("lual_tolstringAddr:[%p] [0x%x] [0x%x]", lual_tolstringAddr, *(size_t *)lual_tolstringAddr, oldlua_tolstring);
    // #endif
    // }
    // by_pointer_t libcHandle = by_dlopen("libc.so", BY_RTLD_LAZY);
    // by_pointer_t gettimeofdayAddr = by_dlsym(libcHandle, "gettimeofday");
    // LOGI("gettimeofdayAddr:%p", gettimeofdayAddr);
    // DobbyHook(gettimeofdayAddr, (dobby_dummy_func_t)gettimeofday_hook, (dobby_dummy_func_t*)&gettimeofday_f);
    return;
}

bool startHacking(JavaVM *vm)
{
    LOGI("fucking start now");
    pthread_t mythread;
    pthread_create(&mythread, nullptr, reinterpret_cast<void *(*)(void *)>(myCheckMap), nullptr);
    // DobbyHook((void*)fopen, (void*)newfopen, (void**)&oldfopen);
    return true;
}