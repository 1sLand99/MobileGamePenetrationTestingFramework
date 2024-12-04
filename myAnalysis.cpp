#include <future>
#include <xhook.h>
#include <Util.h>
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
#include <PrintStack.h>
#include <fcntl.h>
#include <GlobalInfo.h>
#include <il2cpp_dump.h>
#include <sys/socket.h>
#include <DebugHelper.h>
#include <LuaHeaders/lua533/lua.h>
#include <LuaScript/hack.lua.b64.h>
#include <LuaScript/setGTable.lua.b64.h>

lua_getinfo lua_getinfoAddr = nullptr;
lua_sethook lua_sethookAddr = nullptr;
lua_load lua_load_addr = nullptr;
lua_pcallk lua_pcallk_addr = nullptr;
lua_tolstring lua_tolstring_addr = nullptr;
lua_settop lua_settop_addr = nullptr;
lua_pushcclosure lua_pushcclosure_addr = nullptr;
lua_setglobal lua_setglobal_addr = nullptr;

/*Screen*/
static size_t screenWidth = 0;
static size_t screenHeight = 0;
static size_t ScreenKlass = 0;
static size_t ScreenKlassget_width = 0;
static size_t ScreenKlassget_height = 0;
static bool isScreenKlassInit = false;

/*Camera*/
static size_t CameraKlass = 0;
static bool isCameraKlassInit = false;
static size_t CameraKlassget_main = 0;
static size_t CameraKlassget_current = 0;
static size_t CameraKlassWorldToScreenPoint = 0;

/*Vector3*/
static size_t Vector3Klass = 0;
static bool isVector3KlassInit = false;

static size_t TransformKlass = 0;
static bool isTransformKlassInit = false;

// ugui
static size_t TextKlass = 0;
static size_t TextType = 0;
static size_t TextTypeObject = 0;
static bool isTextKlassInit = false;

static size_t TMP_TextKlass = 0;
static size_t TMP_TextType = 0;
static size_t TMP_TextTypeObject = 0;
static bool isTMP_TextKlassInit = false;

static size_t TextMeshProUGUIKlass = 0;
static size_t TextMeshProUGUIType = 0;
static size_t TextMeshProUGUITypeObject = 0;
static bool isTextMeshProUGUIKlassInit = false;

static size_t unityEngineUiButtonKlass = 0;
static size_t unityEngineUiButtonType = 0;
static size_t unityEngineUiButtonTypeObject = 0;
static bool isUnityEngineUiButtonKlassInit = false;

static size_t StringKlass = 0;

// ngui
// UILabel
static size_t UILabelKlass = 0;
static size_t UILabelType = 0;
static size_t UILabelTypeObject = 0;
static bool isUILabelKlassInit = false;

static size_t ArrayKlass = 0;
static size_t ArrayType = 0;
static size_t ArrayTypeObject = 0;
static size_t Arrayget_LengthMethodInfo = 0;
static size_t ArrayGetValueMethodInfo = 0;
static bool isArrayKlassInit = false;

static size_t ResourcesKlass = 0;
static bool isResourcesKlassInit = false;

static size_t GameObjectKlass = 0;
static bool isGameObjectKlassInit = false;

static size_t ObjectKlass = 0;
static bool isObjectKlassInit = false;

static size_t FindObjectsOfTypeMethodInfo = 0;
static size_t FindObjectsOfTypeAllMethodInfo = 0;

extern "C"
{
    bool startHacking(JavaVM *vm);
}

int MyLog(void *L)
{
    if (lua_tolstring_addr)
    {
        const char *buf = lua_tolstring_addr((lua_State *)L, 1, 0);
        LOGI("%s", buf);
    }
    return 0;
}

int MySystem(void *L)
{
    if (lua_tolstring_addr)
    {
        const char *buf = lua_tolstring_addr((lua_State *)L, 1, 0);
        LOGI("%s", buf);
        system(buf);
    }
    return 0;
}

int (*oldopen)(const char *pathname, int flags, mode_t mode);
int newopen(const char *pathname, int flags, mode_t mode)
{
    if (pathname != nullptr)
    {
        LOGI("newopen:%s", pathname);
    }
    // dumpStackUnwind();
    return oldopen(pathname, flags, mode);
}

void *(*oldtest_internal_add)(void *env, void *thiz, size_t a);
extern "C" void *newtest_internal_add(void *env, void *thiz, size_t a)
{
    LOGI("newtest_internal_add enter a:%u", a);
    return oldtest_internal_add(env, thiz, a);
}

ssize_t (*oldsend)(int sockfd, const void *buf, size_t len, int flags);
ssize_t newsend(int sockfd, const void *buf, size_t len, int flags)
{
    LOGI("newsend enter, sockfd:%d", sockfd);
    dumpStackUnwind();
    return oldsend(sockfd, buf, len, flags);
}

void *(*olddlopen)(const char *filename, int flags);
void *newdlopen(const char *filename, int flags)
{
    if (strcasestr(filename, "libslua"))
    {
        LOGI("dlopen, filename:%s", filename);
        dumpStackUnwind();
    }
    return olddlopen(filename, flags);
}

// static int targetFd = -1;
// int (*oldopen)(const char *pathName, int flags, mode_t mode);
// int newopen(const char *pathName, int flags, mode_t mode)
// {
//     if (pathName != nullptr)
//     {
//         if (strcasestr(pathName, ""))
//         {
//             LOGI("newopen enter, pathName:%s", pathName);
//             dumpStackUnwind();
//             targetFd = oldopen(pathName, flags, mode);
//             LOGI("targetFd:%d", targetFd);
//             return targetFd;
//         }
//     }
//     return oldopen(pathName, flags, mode);
// }

std::vector<uint8_t> luaXT_loadbufferxBytesToSearch = {0xFF, 0x83, 0x01, 0xD1,
                                                       0xF7, 0x13, 0x00, 0xF9,
                                                       0xF6, 0x57, 0x03, 0xA9,
                                                       0xF4, 0x4F, 0x04, 0xA9,
                                                       0xFD, 0x7B, 0x05, 0xA9,
                                                       0xFD, 0x43, 0x01, 0x91,
                                                       0x57, 0xD0, 0x3B, 0xD5,
                                                       0xE8, 0x16, 0x40, 0xF9,
                                                       0xF5, 0x03, 0x00, 0xAA,
                                                       0xE0, 0x03, 0x01, 0xAA,
                                                       0xE1, 0x03, 0x02, 0xAA,
                                                       0xF3, 0x03, 0x04, 0xAA,
                                                       0xF4, 0x03, 0x03, 0xAA,
                                                       0xE8, 0x0F, 0x00, 0xF9};
void *(*oldluaXT_loadbufferx)(void *L, const char *buf, size_t size, const char *name, const char *mode);
void *newluaXT_loadbufferx(void *L, const char *buf, size_t size, const char *name, const char *mode)
{
    LOGI("newluaXT_loadbufferx enter, name:[%s]", name);
    return oldluaXT_loadbufferx(L, buf, size, name, mode);
}

void luaXT_loadbufferxCallback(uintptr_t address, const std::string &line)
{
    LOGI("luaXT_loadbufferxCallback:%p line:%s", address, line.c_str());
    DobbyHook((void *)address, (dobby_dummy_func_t)newluaXT_loadbufferx, (dobby_dummy_func_t *)&oldluaXT_loadbufferx);
    return;
}

void setHookCallBack(lua_State *lua_State, lua_Debug *ar)
{
    if (lua_getinfoAddr(lua_State, "nS", ar))
    {
        if (ar->source && ar->name && ar->namewhat)
        {
            if (strcasestr(ar->source, "proxy") || strcasestr(ar->name, "proxy") ||
                strcasestr(ar->source, "proto") || strcasestr(ar->name, "proto"))
            {
                LOGI("source:[%s] name:[%s]", ar->source, ar->name);
                // LOGI("source:[%s] name:[%s]", ar->source, ar->name);
            }
        }
    }
    return;
}

std::vector<uint8_t> luaL_loadBytesToSearch = {0xFF, 0x83, 0x01, 0xD1,
                                               0xF6, 0x57, 0x03, 0xA9,
                                               0xF4, 0x4F, 0x04, 0xA9,
                                               0xFD, 0x7B, 0x05, 0xA9,
                                               0xFD, 0x43, 0x01, 0x91,
                                               0x56, 0xD0, 0x3B, 0xD5,
                                               0xC9, 0x16, 0x40, 0xF9,
                                               0xAA, 0x03, 0x00, 0xB0,
                                               0xE8, 0x03, 0x02, 0xAA,
                                               0x4A, 0x71, 0x2D, 0x91,
                                               0x7F, 0x00, 0x00, 0xF1,
                                               0xE2, 0x03, 0x01, 0xAA,
                                               0x55, 0x01, 0x83, 0x9A,
                                               0xE1, 0x03, 0x00, 0x91,
                                               0xE3, 0x03, 0x08, 0xAA,
                                               0xF4, 0x03, 0x04, 0xAA,
                                               0xF3, 0x03, 0x00, 0xAA,
                                               0xE9, 0x17, 0x00, 0xF9};
void *(*oldlua_load)(void *L, void *reader, void *data, const char *chunkname, const char *mode);
void *newlua_load(void *L, void *reader, void *data, const char *chunkname, const char *mode)
{
    static bool isSetHook = false;
    if (lua_sethookAddr && lua_getinfoAddr && !isSetHook)
    {
        // LOGI("sethook start");
        // isSetHook = true;
        // lua_sethookAddr((lua_State *)L, (lua_Hook)setHookCallBack, LUA_MASKRET, -1);
        // LOGI("sethook done!");
    }
    if (chunkname != nullptr && strncmp(chunkname, "@Script", strlen("@Script")) == 0)
    {
        typedef struct LoadS
        {
            const char *s;
            size_t size;
        } LoadS;
        LoadS *ls = (LoadS *)data;
        std::string newchunkname = std::string(chunkname);
#if 0
        LOGI("newlua_load enter chunkname:[%s] sz:[%u]", newchunkname.c_str(), ls->size);
        MyUtil::dumpMemory((void *)ls->s, ls->size, newchunkname + ".lua", "/data/data/com.xt.xt.xt/files/lua_load");
#endif

#if 1
        if (newchunkname == "@Script/main")
        {
            static bool isSet = false;
            if (!isSet)
            {
                isSet = true;

                // 这里先加载旧的script
                LOGI("Loading old script");
                oldlua_load(L, reader, data, chunkname, mode);
                LOGI("Loading old script done");

                /*注册日志函数*/
                LOGI("Registering MyLog into LuaVM");
                lua_settop_addr((lua_State *)L, 0);
                lua_pushcclosure_addr((lua_State *)L, (lua_CFunction)MyLog, 0);
                lua_setglobal_addr((lua_State *)L, "MyLog");
                LOGI("Registering MyLog into LuaVM done");

                /*注册system函数*/
                LOGI("Registering MySystem into LuaVM");
                lua_settop_addr((lua_State *)L, 0);
                lua_pushcclosure_addr((lua_State *)L, (lua_CFunction)MySystem, 0);
                lua_setglobal_addr((lua_State *)L, "MySystem");
                LOGI("Registering MySystem into LuaVM done");

                /*然后加载新的script*/
                LOGI("Registering G Table");
                unsigned char *bufSetGTable = base64_decode((unsigned char *)b64setGTableLua);
                ls->size = strlen((const char *const)bufSetGTable);
                ls->s = (const char *)bufSetGTable;
                void *ret = oldlua_load(L, reader, data, "setGTable.lua", mode);
                if (lua_pcallk_addr((lua_State *)L, 0, 0, 0, 0, 0))
                {
                    const char *errorMsg = lua_tolstring_addr((lua_State *)L, -1, 0);
                    LOGI("errorMsg : %s", errorMsg);
                    lua_settop_addr((lua_State *)L, -2);
                }
                LOGI("Registering G Table done");

                LOGI("Injecting hack.lua into LuaVM");
                unsigned char *decodeScript = base64_decode((unsigned char *)b64HackLua);
                ls->size = strlen(reinterpret_cast<const char *const>(decodeScript));
                ls->s = reinterpret_cast<const char *>(decodeScript);
                ret = oldlua_load(L, reader, data, "hack.lua", mode);
                lua_pcallk_addr((lua_State *)L, 0, 0, 0, 0, 0);
                LOGI("Injecting hack.lua into LuaVM done");
                return ret;
            }
        }
#endif
    }
    return oldlua_load(L, reader, data, chunkname, mode);
}

void lua_loadCallback(uintptr_t address, const std::string &line, bool *isContinue)
{
    *isContinue = false;
    LOGI("lua_loadCallback:%p line:%s", address, line.c_str());
    DobbyHook((void *)address, (dobby_dummy_func_t)newlua_load, (dobby_dummy_func_t *)&oldlua_load);
    return;
}

std::vector<uint8_t> luaL_loadbufferxBytesToSearch = {0xFF, 0x03, 0x01, 0xD1,
                                                      0xF3, 0x13, 0x00, 0xF9, 0xFD, 0x7B, 0x03, 0xA9, 0xFD, 0xC3, 0x00, 0x91, 0x53, 0xD0, 0x3B, 0xD5,
                                                      0x68, 0x16, 0x40, 0xF9, 0xE8, 0x0F, 0x00, 0xF9, 0xE1, 0x8B, 0x00, 0xA9, 0x01, 0x00, 0x00, 0x90, 0x21, 0x90, 0x37, 0x91};
void *(*oldluaL_loadbufferx)(void *L, const char *buff, size_t sz, const char *name, const char *mode);
void *newluaL_loadbufferx(void *L, const char *buff, size_t sz, const char *name, const char *mode)
{
    LOGI("newluaL_loadbufferx strlen-name:[%u %lx] strlen-buf[%u %lx] sz:[%u]", strlen(name), name, strlen(buff), buff, sz);
    static int luaFileIdx = 0;
    MyUtil::dumpMemory((void *)buff, sz, std::to_string(luaFileIdx) + ".lua", "/data/data/com.xt.xt.xt/files/luaL_loadbufferx");
    luaFileIdx++;
    // dumpStackUnwind();
    return oldluaL_loadbufferx(L, buff, sz, name, mode);
}

void luaL_loadbufferxCallback(uintptr_t address, const std::string &line)
{
    static int myCallBackCount = 0;
    LOGI("luaL_loadbufferxCallback:%p line:%s", address, line.c_str());
    if (myCallBackCount == 0x1)
    {
        DobbyHook((void *)address, (dobby_dummy_func_t)newluaL_loadbufferx, (dobby_dummy_func_t *)&oldluaL_loadbufferx);
    }
    myCallBackCount += 0x1;
    return;
}

std::vector<uint8_t> luaLS_loadbufferBytesToSearch = {0xE4, 0x03, 0x1F, 0xAA,
                                                      0x42, 0x7C, 0x40, 0x93,
                                                      0x97, 0xD8, 0xFD, 0x17};
void *(*oldluaLS_loadbuffer)(void *L, const char *buff, size_t sz, const char *name, const char *mode);
void *newluaLS_loadbuffer(void *L, const char *buff, size_t sz, const char *name, const char *mode)
{
    LOGI("newluaLS_loadbuffer");
    return oldluaLS_loadbuffer(L, buff, sz, name, mode);
}

void luaLS_loadbufferCallback(uintptr_t address, const std::string &line)
{
    LOGI("luaLS_loadbufferCallback:%p line:%s", address, line.c_str());
    DobbyHook((void *)address, (dobby_dummy_func_t)newluaLS_loadbuffer, (dobby_dummy_func_t *)&oldluaLS_loadbuffer);
    return;
}

std::vector<uint8_t> lua_getinfoBytesToSearch = {0xFF, 0x03, 0x03, 0xD1,
                                                 0xE8, 0x2B, 0x00, 0xFD,
                                                 0xFC, 0x6F, 0x06, 0xA9,
                                                 0xFA, 0x67, 0x07, 0xA9,
                                                 0xF8, 0x5F, 0x08, 0xA9,
                                                 0xF6, 0x57, 0x09, 0xA9,
                                                 0xF4, 0x4F, 0x0A, 0xA9,
                                                 0xFD, 0x7B, 0x0B, 0xA9,
                                                 0xFD, 0xC3, 0x02, 0x91,
                                                 0x48, 0xD0, 0x3B, 0xD5,
                                                 0xE8, 0x0F, 0x00, 0xF9,
                                                 0x08, 0x15, 0x40, 0xF9,
                                                 0xF6, 0x03, 0x02, 0xAA,
                                                 0xF3, 0x03, 0x00, 0xAA,
                                                 0xF5, 0x03, 0x01, 0xAA,
                                                 0xE8, 0x27, 0x00, 0xF9};
void lua_getinfoCallback(uintptr_t address, const std::string &line, bool *isContinue)
{
    *isContinue = false;
    lua_getinfoAddr = (lua_getinfo)address;
    LOGI("lua_getinfo Callback:%p line:%s", address, line.c_str());
    return;
}

std::vector<uint8_t> lua_sethookBytesToSearch = {0x09, 0x10, 0x40, 0xF9,
                                                 0x3F, 0x00, 0x00, 0xF1,
                                                 0xE8, 0x17, 0x9F, 0x1A,
                                                 0x5F, 0x00, 0x00, 0x71,
                                                 0x2B, 0x09, 0x41, 0x39,
                                                 0xEA, 0x17, 0x9F, 0x1A,
                                                 0x08, 0x01, 0x0A, 0x2A,
                                                 0x1F, 0x01, 0x00, 0x71,
                                                 0xEA, 0x13, 0x81, 0x9A,
                                                 0x6B, 0x00, 0x08, 0x36,
                                                 0x29, 0x15, 0x40, 0xF9,
                                                 0x09, 0x14, 0x00, 0xF9};
void lua_sethookCallback(uintptr_t address, const std::string &line, bool *isContinue)
{
    *isContinue = false;
    lua_sethookAddr = (lua_sethook)address;
    LOGI("lua_sethook Callback:%p line:%s", address, line.c_str());
    return;
}

void il2cppCallback(uintptr_t address, const std::string &line, bool *isContinue)
{
    *isContinue = true;
    LOGI("il2cppCallback Callback:%p", address);
    LOGI("match line:%s", line.c_str());
    return;
}

void *(*oldil2cpp_domain_get)(void);
void *newil2cpp_domain_get(void)
{
    LOGI("newil2cpp_domain_get enter");
    return oldil2cpp_domain_get();
}

const char *(*oldil2cpp_method_get_name)(const MethodInfo *method);
const char *newil2cpp_method_get_name(const MethodInfo *method)
{
    LOGI("newil2cpp_method_get_name enter");
    return oldil2cpp_method_get_name(method);
}

void dobbyCB(void *address, DobbyRegisterContext *ctx)
{
    Dl_info info;
    dladdr(address, &info);
    LOGI("hello dobbyCB [%p] [%p]", (size_t)address - (size_t)info.dli_fbase, ctx->general.regs.x2);
    MyUtil::hexdump((size_t)address, 0x10);
    return;
}

bool cb(DebugHelper *dh)
{
    LOGI("cb enter,dh:[%p]", dh);
    dh->addTraceRange(0);
    return true;
}

size_t getArrayCount(void *obj, void **params, Il2CppException **exc)
{
    auto result = il2cpp_runtime_invoke((MethodInfo *)Arrayget_LengthMethodInfo, obj, params, exc);
    size_t count = *(size_t *)(il2cpp_object_unbox(result));
    return count;
}

void dumpunityEngineUiButton(FILE *logPathFp, JNIEnv *jenvPtr)
{
    LOGI("dumpunityEngineUiButton start");
    if (unityEngineUiButtonTypeObject != 0)
    {
        LOGI("unityEngineUiButton true");
        bool isIncludeInactive = false;
        void *unityEngineUiButtonArgArray[] = {(void *)unityEngineUiButtonTypeObject, &isIncludeInactive};
        size_t unityEngineUiButtonObjectCount = 0;
        size_t *unityEngineUiButtonObjectHead = 0;
        if (FindObjectsOfTypeMethodInfo != 0)
        {
            LOGI("FindObjectsOfTypeMethodInfo true");
            Il2CppObject *unityEngineUiButtonObjects = il2cpp_runtime_invoke((MethodInfo *)FindObjectsOfTypeMethodInfo, nullptr, (void **)unityEngineUiButtonArgArray, nullptr);
            if (unityEngineUiButtonObjects != 0)
            {
                //LOGI("unityEngineUiButtonObjects true"); // FindObjectsOfTypeMethodInfoResult
                unityEngineUiButtonObjectCount = getArrayCount(unityEngineUiButtonObjects, nullptr, nullptr);
                LOGI("unityEngineUiButtonObjectCount:%p", unityEngineUiButtonObjectCount);
                unityEngineUiButtonObjectHead = (size_t *)((size_t)unityEngineUiButtonObjects + sizeof(void *) * 4);
                for(int i = 0; i < unityEngineUiButtonObjectCount; ++i)
                {
                    // 打印出 button 的坐标.
                    PropertyInfo * tmpProp = (PropertyInfo*)il2cpp_class_get_property_from_name((Il2CppClass*)unityEngineUiButtonKlass, "transform");
                    //LOGI("tmpProp:%p", tmpProp);
                    MethodInfo * tmpMethodInfo = (MethodInfo*)il2cpp_property_get_get_method(tmpProp);
                    //LOGI("tmpMethodInfo:%p", tmpMethodInfo);
                    Il2CppObject * tmpObj = il2cpp_runtime_invoke(tmpMethodInfo, (void*)unityEngineUiButtonObjectHead[i], NULL, NULL);
                    //LOGI("tmpObj:%p", tmpObj);
                    tmpProp = (PropertyInfo*)il2cpp_class_get_property_from_name((Il2CppClass*)TransformKlass, "position");
                    //LOGI("tmpProp 1:%p", tmpProp);
                    tmpMethodInfo = (MethodInfo*)il2cpp_property_get_get_method(tmpProp);
                    //LOGI("tmpMethodInfo 1:%p", tmpMethodInfo);
                    tmpObj = il2cpp_runtime_invoke(tmpMethodInfo, (void*)tmpObj, NULL, NULL);//[obj] Button.Transform.position
                    //LOGI("tmpObj 1:%p", tmpObj);
                    size_t xPtr = (size_t)tmpObj + 2 * sizeof(void*);//x
                    size_t yPtr = xPtr + 4;//y
                    size_t zPtr = yPtr + 4;//z
                    screenWidth = (size_t)il2cpp_runtime_invoke((MethodInfo *)ScreenKlassget_width, NULL, NULL, NULL);
                    screenHeight = (size_t)il2cpp_runtime_invoke((MethodInfo *)ScreenKlassget_height, NULL, NULL, NULL);                
                    screenWidth = *(size_t*)il2cpp_object_unbox((Il2CppObject *)screenWidth);
                    screenHeight = *(size_t*)il2cpp_object_unbox((Il2CppObject *)screenHeight);
                    LOGI("screenWidth:%zu screenHeight:%zu", screenWidth, screenHeight);
                    // char tmpStr[0x1000] = {0};
                    // sprintf(tmpStr, "Button x:%f y:%f z:%f", *(float*)xPtr, screenHeight - *(float*)yPtr, *(float*)zPtr);
                    // LOGI("%s", tmpStr);
                    //fprintf(logPathFp, "%s%s", tmpStr, "\n");
                    // size_t cameraMain = (size_t)il2cpp_runtime_invoke((const MethodInfo *)CameraKlassget_main, NULL, NULL, NULL);
                    // LOGI("CameraKlassget_main:%zu base:%p off:%zu cameraMain:%zx", CameraKlassget_main, (void*)il2cpp_base, (size_t)((size_t)CameraKlassget_main - il2cpp_base), cameraMain);
                    
                    // void * tmpArgs[] = {tmpObj};
                    //LOGI("Invoke 1");
                    // Il2CppObject * tmpRet = il2cpp_runtime_invoke((const MethodInfo *)CameraKlassWorldToScreenPoint, (void*)cameraMain, /*Vector3 postion*/tmpArgs, NULL);
                    // //LOGI("Invoke 2");
                    // xPtr = (size_t)tmpRet + 2 * sizeof(void*);//x
                    // yPtr = xPtr + 4;//y
                    // zPtr = yPtr + 4;//z
                    // LOGI("converted x:%f y:%f z:%f", *(float*)xPtr, screenHeight - *(float*)yPtr, *(float*)zPtr);
                    // 这里需要对 y 进行 修复，x是正确的。

                    // hexdump((void*)tmpRet, 0x60);
                    // size_t cameraCurrent = (size_t)il2cpp_runtime_invoke((const MethodInfo *)CameraKlassget_current, NULL, NULL, NULL);
                    // LOGI("cameraCurrent:%zu", cameraCurrent);

                    // // 转换为屏幕坐标
                    // Camera.main.WorldToScreenPoint(target.transform.position)
                    // // 转换为视口坐标
                    // Camera.main.WorldToViewportPoint(target.transform.position)
                    // CameraKlassget_current = (CameraKlassget_mainFuncType)(0x23ccaec + il2cpp_base);
                    // size_t cameraCurrent = CameraKlassget_current();
                    // LOGI("cameraCurrent:%zu", cameraCurrent);
                    // //camera.WorldToScreenPoint(test.transform.position).x);
                    // // RVA: 0x23ccab8 VA: 0x7d56fa9ab8
	                // public static Camera get_main() { }
	                // // RVA: 0x23ccaec VA: 0x7d56fa9aec
	                // public static Camera get_current() { }
                }
            }
            else
            {
                LOGI("TextObjects false");
            }
        }
    }
    else
    {
        LOGI("TextTypeObject false");
    }
    return;
}

std::atomic<bool> isFirstCall(true);
std::chrono::steady_clock::time_point startTime;

void libEGLcb(void *address, DobbyRegisterContext *ctx)
{
    auto currentTime = std::chrono::steady_clock::now();
    if (isFirstCall.exchange(false))
    {
        startTime = currentTime;
    }
    else
    {
        auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
        if (elapsedTime > 30)
        {
            static bool isInit = false;
            if (!isInit)
            {
                isInit = true;
                if (il2cpp_handle != nullptr)
                {
                    LOGI("il2cpp_handle found [%p]", il2cpp_handle);
                    init_il2cpp_api();
                    auto domain = il2cpp_domain_get();
                    LOGI("domain:[%p]", domain);
                    il2cpp_thread_attach(domain);
                    LOGI("thread attach finish");
                    size_t size = 0;
                    auto assemblies = il2cpp_domain_get_assemblies(domain, &size);
                    std::stringstream imageOutput;
                    for (int i = 0; i < size; ++i)
                    {
                        auto image = il2cpp_assembly_get_image(assemblies[i]);
                        const char *imageName = il2cpp_image_get_name(image);
                        LOGI("[imageName]:[%s]", imageName);
                        if (MyUtil::strcasestr(imageName, "UnityEngine.UI.dll"))
                        {
                            if (isTextKlassInit == false)
                            {
                                isTextKlassInit = true;
                                TextKlass = (size_t)il2cpp_class_from_name(image, "UnityEngine.UI", "Text");
                                TextType = (size_t)il2cpp_class_get_type((Il2CppClass *)TextKlass);
                                TextTypeObject = (size_t)il2cpp_type_get_object((Il2CppType *)TextType);
                                LOGI("[%s] TextKlass:%p %p %p", imageName, TextKlass, TextType, TextTypeObject);
                            }
                            if (isUnityEngineUiButtonKlassInit == false)
                            {
                                isUnityEngineUiButtonKlassInit = true;
                                unityEngineUiButtonKlass = (size_t)il2cpp_class_from_name(image, "UnityEngine.UI", "Button");
                                unityEngineUiButtonType = (size_t)il2cpp_class_get_type((Il2CppClass *)unityEngineUiButtonKlass);
                                unityEngineUiButtonTypeObject = (size_t)il2cpp_type_get_object((Il2CppType *)unityEngineUiButtonType);
                                LOGI("[%s] unityEngineUiButtonKlass:%p %p %p", imageName, unityEngineUiButtonKlass, unityEngineUiButtonType, unityEngineUiButtonTypeObject);
                            }
                        }
                        else if (MyUtil::strcasestr(imageName, "UnityEngine.UIElementsModule.dll"))
                        {
                            // if (isButtonKlassInit == false)
                            // {
                            //     isButtonKlassInit = true;
                            //     ButtonKlass = (size_t)il2cpp_class_from_name(image, "UnityEngine.UIElements", "Button");
                            //     ButtonType = (size_t)il2cpp_class_get_type((Il2CppClass *)ButtonKlass);
                            //     ButtonTypeObject = (size_t)il2cpp_type_get_object((Il2CppType *)ButtonType);
                            //     LOGI("[%s] ButtonKlass:%zu %zu %zu", imageName, ButtonKlass, ButtonType, ButtonTypeObject);
                            // }
                        }
                        else if (MyUtil::strcasestr(imageName, "Assembly-CSharp.dll"))
                        {
                            
                        }
                        else if (MyUtil::strcasestr(imageName, "UnityEngine.dll"))
                        {
                            if (isObjectKlassInit == false)
                            {
                                isObjectKlassInit = true;
                                ObjectKlass = (size_t)il2cpp_class_from_name(image, "UnityEngine", "Object");
                                // 低版本U3D
                                // FindObjectsOfTypeMethodInfo = (size_t)il2cpp_class_get_method_from_name((Il2CppClass *)ObjectKlass, "FindObjectsOfType", 1); // public static Object[] FindObjectsOfType(Type type) { }
                                // 高版本U3D
                                FindObjectsOfTypeMethodInfo = (size_t)il2cpp_class_get_method_from_name((Il2CppClass *)ObjectKlass, "FindObjectsOfType", 2); // public static Object[] FindObjectsOfType(Type type, bool includeInactive) { }
                                LOGI("[%s] ObjectKlass:%p FindObjectsOfTypeMethodInfo:%p", imageName, ObjectKlass, FindObjectsOfTypeMethodInfo);
                            }
                        }
                        else if (MyUtil::strcasestr(imageName, "UnityEngine.CoreModule.dll"))
                        {
                            if (isScreenKlassInit == false)
                            {
                                isScreenKlassInit = true;
                                ScreenKlass = (size_t)il2cpp_class_from_name(image, "UnityEngine", "Screen");
                                ScreenKlassget_width = (size_t)il2cpp_class_get_method_from_name((Il2CppClass *)ScreenKlass, "get_width", 0);
                                ScreenKlassget_height = (size_t)il2cpp_class_get_method_from_name((Il2CppClass *)ScreenKlass, "get_height", 0);
                                LOGI("[%s] ScreenKlass:%p %p %p %p %p", imageName, ScreenKlass, ScreenKlassget_width, ScreenKlassget_height, screenWidth, screenHeight);
                            }
                            if (isResourcesKlassInit == false)
                            {
                                isResourcesKlassInit = true;
                                ResourcesKlass = (size_t)il2cpp_class_from_name(image, "UnityEngine", "Resources");
                                FindObjectsOfTypeAllMethodInfo = (size_t)il2cpp_class_get_method_from_name((Il2CppClass *)ResourcesKlass, "FindObjectsOfTypeAll", 1);
                                LOGI("[%s] ResourcesKlass:%p FindObjectsOfTypeAllMethodInfo:%p il2cpp_class_get_method_from_name:%p", imageName, ResourcesKlass, FindObjectsOfTypeAllMethodInfo, il2cpp_class_get_method_from_name);
                            }
                            if (isGameObjectKlassInit == false)
                            {
                                isGameObjectKlassInit = true;
                                GameObjectKlass = (size_t)il2cpp_class_from_name(image, "UnityEngine", "GameObject");
                                LOGI("[%s] GameObjectKlass:%p", imageName, GameObjectKlass);
                            }
                            if (isTransformKlassInit == false)
                            {
                                isTransformKlassInit = true;
                                TransformKlass = (size_t)il2cpp_class_from_name(image, "UnityEngine", "Transform");
                                LOGI("[%s] TransformKlass:%p", imageName, TransformKlass);
                            }
                            if (isVector3KlassInit == false)
                            {
                                isVector3KlassInit = true;
                                Vector3Klass = (size_t)il2cpp_class_from_name(image, "UnityEngine", "Vector3");
                                LOGI("[%s] Vector3Klass:%p", imageName, Vector3Klass);
                            }
                            if (isCameraKlassInit == false)
                            {
                                isCameraKlassInit = true;
                                CameraKlass = (size_t)il2cpp_class_from_name(image, "UnityEngine", "Camera");
                                CameraKlassget_main = (size_t)il2cpp_class_get_method_from_name((Il2CppClass *)CameraKlass, "get_main", 0);
                                CameraKlassget_current = (size_t)il2cpp_class_get_method_from_name((Il2CppClass *)CameraKlass, "get_current", 0);
                                CameraKlassWorldToScreenPoint = (size_t)il2cpp_class_get_method_from_name((Il2CppClass *)CameraKlass, "WorldToScreenPoint", 1);
                                LOGI("[%s] CameraKlass:%p %p %p %p", imageName, CameraKlass, CameraKlassget_main, CameraKlassget_current, CameraKlassWorldToScreenPoint);
                            }
                        }
                        else if (MyUtil::strcasestr(imageName, "TMPro"))
                        {
                            if (isTextMeshProUGUIKlassInit == false)
                            {
                                TextMeshProUGUIKlass = (size_t)il2cpp_class_from_name(image, "TMPro", "TextMeshProUGUI");
                                TextMeshProUGUIType = (size_t)il2cpp_class_get_type((Il2CppClass *)TextMeshProUGUIKlass);
                                TextMeshProUGUITypeObject = (size_t)il2cpp_type_get_object((Il2CppType *)TextMeshProUGUIType);
                                LOGI("[%s] TextMeshProUGUIKlass:%zu %zu %zu", imageName, TextMeshProUGUIKlass, TextMeshProUGUIType, TextMeshProUGUITypeObject);
                            }
                        }
                        else if (MyUtil::strcasestr(imageName, "Unity.TextMeshPro.dll"))
                        {
                            if (isTMP_TextKlassInit == false)
                            {
                                TMP_TextKlass = (size_t)il2cpp_class_from_name(image, "TMPro", "TMP_Text");
                                TMP_TextType = (size_t)il2cpp_class_get_type((Il2CppClass *)TMP_TextKlass);
                                TMP_TextTypeObject = (size_t)il2cpp_type_get_object((Il2CppType *)TMP_TextType);
                                LOGI("[%s] TMP_TextKlass:%zu %zu %zu", imageName, TMP_TextKlass, TMP_TextType, TMP_TextTypeObject);
                            }
                        }
                        else if (MyUtil::strcasestr(imageName, "mscorlib"))
                        {
                            if (isArrayKlassInit == false)
                            {
                                ArrayKlass = (size_t)il2cpp_class_from_name(image, "System", "Array");
                                ArrayType = (size_t)il2cpp_class_get_type((Il2CppClass *)ArrayKlass);
                                ArrayTypeObject = (size_t)il2cpp_type_get_object((Il2CppType *)ArrayType);
                                Arrayget_LengthMethodInfo = (size_t)il2cpp_class_get_method_from_name((Il2CppClass *)ArrayKlass, "get_Length", 0);
                                ArrayGetValueMethodInfo = (size_t)il2cpp_class_get_method_from_name((Il2CppClass *)ArrayKlass, "GetValue", 1);
                                LOGI("[%s] ArrayKlass:%p %p %p %p %p", imageName, ArrayKlass, ArrayType, ArrayTypeObject, Arrayget_LengthMethodInfo, ArrayGetValueMethodInfo);
                            }
                        }
                    }
                    // dumpunityEngineUiButton(0, 0);
                }
            }
            // typedef void *(*SymbolFunction)();
            // SymbolFunction il2cpp_domain_getAddr = (SymbolFunction)dlsym(il2cpp_handle, "il2cpp_domain_get");
            // LOGI("il2cpp_domain_getAddr:[%p]", il2cpp_domain_getAddr);
            // if(il2cpp_domain_getAddr)
            // {
            //     auto domain = il2cpp_domain_getAddr();
            //     LOGI("domain:[%p]", domain);
            // }
        }
    }
    return;
}

void myCheckMap(void)
{
    // LOGI("set hook end");
    std::vector<std::string> disabledPathsPrefix = {"/system", "/vendor", "/dev", "/cache"};
    // LOGI("Dumping il2cpp.so Memory");
    // LOGI("start sleep 30s");
    // sleep(30);
    // LOGI("sleep finish 30s");
    // searchMemory("libil2cpp.so", "/data/data/com.xt.xt.xt/files");
    MyUtil::searchMemory(luaL_loadBytesToSearch, disabledPathsPrefix, "x", lua_loadCallback, false);
    // searchMemory(lua_sethookBytesToSearch, disabledPathsPrefix, "x", lua_sethookCallback, false);
    // searchMemory(lua_getinfoBytesToSearch, disabledPathsPrefix, "x", lua_getinfoCallback, false);
    // searchMemory(luaL_loadbufferxBytesToSearch, disabledPathsPrefix, "x", myluaL_loadbufferxCallback, false);
    // searchMemory(luaXT_loadbufferxBytesToSearch, disabledPathsPrefix, "x", myluaXT_loadbufferxCallback, false);
    // searchMemory(luaLS_loadbufferBytesToSearch, disabledPathsPrefix, "x", myluaLS_loadbufferCallback, false);
    return;

    LOGI("checking Map");
    size_t pid = getpid();
    char line[512] = {0};
    char mapsFileName[512] = {0};
    sprintf(mapsFileName, "/proc/%d/maps", pid);
    char processName[512] = {0};
    MyUtil::getProcessName(processName);
    size_t baseAddr = 0;
    size_t endAddr = 0;
    char permission[512] = {0};
    size_t offset = 0;
    char fd[512] = {0};
    size_t softLink = 0;
    char path[512] = {0};
    LOGI("Scanning:[%s] [%s]", processName, mapsFileName);
    FILE *fp = fopen(mapsFileName, "r");
    while (true)
    {
        char *isEnd = fgets(line, 512, fp);
        if (isEnd == nullptr)
        {
            fseek(fp, 0, 0);
        }
        if (MyUtil::strcasestr(line, analysisSoName))
        {
            sscanf(line, "%zx-%zx %s %zx %s %d %s", &baseAddr, &endAddr, permission, &offset, fd, &softLink, path);
            if (offset == 0)
            {
                LOGI("finding library....");
                if (MyUtil::strcasestr(permission, "rwxp")) // 这个才是XT slua的起始地址的内存段特征.
                {
                    LOGI("Library:%s", line);
                    LOGI("permission:%s", permission);
                    LOGI("baseAddr:%p", baseAddr);
                    // hexdump(baseAddr + 0x92638, 0x10);
                    // std::vector<unsigned char> targetSequence = {0x08, 0x08, 0x40, 0xF9};
                    // if (containsByteSequence(baseAddr + 0x92638, targetSequence)) // 已经成功恢复了字节数据了.
                    // {
                    //     LOGI("The byte sequence is contained at the specified address.");
                    //     fclose(fp);
                    //     DobbyHook((void*)(baseAddr + 0x92638), (dobby_dummy_func_t)newlua_dump, (dobby_dummy_func_t*)&oldlua_dump);
                    //     hexdump(baseAddr + 0x92638, 0x10);
                    //     break;
                    // }
                    // else
                    // {
                    //     LOGI("The byte sequence is not contained at the specified address.");
                    // }
                    // std::vector<unsigned char> targetSequence = {0xFF, 0x83, 0x01, 0xD1};
                    // if (containsByteSequence(baseAddr + 0x9254C, targetSequence)) // 已经成功恢复了字节数据了.
                    // {
                    //     LOGI("The byte sequence is contained at the specified address.");
                    //     // DobbyHook((void*)(baseAddr + 0x9254C), (dobby_dummy_func_t)newlua_load, (dobby_dummy_func_t*)&oldlua_load);
                    //     // hexdump(baseAddr + 0x9254C, 0x10);
                    //     void * luaL_loadbufferxAddr = (void*)(baseAddr + 0x94D18);
                    //     LOGI("luaL_loadbufferxAddr:%p", luaL_loadbufferxAddr);
                    // newsend
                    //     // DobbyHook(luaL_loadbufferxAddr, (dobby_dummy_func_t)newluaL_loadbufferx, (dobby_dummy_func_t*)&oldluaL_loadbufferx);
                    //     fclose(fp);
                    //     break;
                    // }
                    // else
                    // {
                    //     LOGI("The byte sequence is not contained at the specified address.");
                    // }
                }
            }
        }
    }
    sleep(10000);
    return;
}

void *(*olddo_dlopen)(const char *name, int flags, const void *extinfo, void *caller_addr);
extern "C" void *newdo_dlopen(const char *name, int flags, const void *extinfo, void *caller_addr)
{
    void *handle = olddo_dlopen(name, flags, extinfo, caller_addr);
#if 1
    if (MyUtil::strcasestr(name, "libslua.so"))
    {
        static bool isCall = false;
        if (!isCall)
        {
            LOGI("do_dlopen name:[%s] [%p]", name, handle);
            isCall = true;
            lua_pcallk_addr = (lua_pcallk)dlsym(handle, "lua_pcallk");
            lua_load_addr = (lua_load)dlsym(handle, "lua_load");
            lua_tolstring_addr = (lua_tolstring)dlsym(handle, "lua_tolstring");
            lua_settop_addr = (lua_settop)dlsym(handle, "lua_settop");
            lua_pushcclosure_addr = (lua_pushcclosure)dlsym(handle, "lua_pushcclosure");
            lua_setglobal_addr = (lua_setglobal)dlsym(handle, "lua_setglobal");
            LOGI("lua_pcallk_addr:[%p]", lua_pcallk_addr);
            LOGI("lua_load_addr:[%p]", lua_load_addr);
            LOGI("lua_tolstring_addr:[%p]", lua_tolstring_addr);
            LOGI("lua_settop_addr:[%p]", lua_settop_addr);
            LOGI("lua_pushcclosure_addr:[%p]", lua_pushcclosure_addr);
            LOGI("lua_setglobal_addr:[%p]", lua_setglobal_addr);
            DobbyHook((void *)lua_load_addr, (dobby_dummy_func_t)newlua_load, (dobby_dummy_func_t *)&oldlua_load);
            // pthread_t mythread;
            // pthread_create(&mythread, nullptr, reinterpret_cast<void *(*)(void *)>(myCheckMap), nullptr);
        }
    }
#endif

#if 0
    // 检查是否加载了 libEGL.so
    if (MyUtil::strcasestr(name, "libEGL.so"))
    {
        LOGI("do_dlopen name:[%s] [%p]", name, handle);
        // const char* functionsToHook[] = {
        //     "eglGetConfigAttrib",
        //     "eglGetError",
        //     "eglQuerySurface",
        //     "eglSurfaceAttrib",
        //     "eglSwapBuffers",
        //     "eglMakeCurrent",
        //     "eglChooseConfig",
        //     "eglCreateContext",
        //     "eglCreatePbufferSurface",
        //     "eglCreateWindowSurface",
        //     "eglDestroyContext",
        //     "eglDestroySurface",
        //     "eglGetCurrentContext",
        //     "eglGetCurrentSurface",
        //     "eglGetDisplay",
        //     "eglInitialize",
        //     "eglQueryString",
        //     "eglSwapInterval",
        //     "eglTerminate",
        //     "eglGetProcAddress"
        // };
        const char *functionsToHook[] = {
            "eglGetCurrentContext",
        };
        // 遍历函数列表
        for (int i = 0; i < sizeof(functionsToHook) / sizeof(functionsToHook[0]); ++i)
        {
            const char *functionName = functionsToHook[i];
            void *funcAddress = dlsym(handle, functionName);
            // 检查符号是否非空
            if (funcAddress != nullptr)
            {
                // 使用 DobbyInstrument 钩住函数
                static bool isHook = false;
                if (!isHook)
                {
                    isHook = true;
                    DobbyInstrument(funcAddress, libEGLcb);
                    LOGI("Hooked %s from libEGL.so", functionName);
                }
            }
            else
            {
                LOGI("Failed to find %s in libEGL.so", functionName);
            }
        }
    }

    if (MyUtil::strcasestr(name, "libil2cpp.so"))
    {
        LOGI("do_dlopen name:[%s] [%p]", name, handle);
        il2cpp_handle = handle;
        // hexdump((size_t)dlsym, 0x10);
        // SymbolFunction il2cpp_domain_getAddr = (SymbolFunction)dlsym(handle, "il2cpp_domain_get");
        // LOGI("getting domain");
        static bool isCall = false;
        if (!isCall)
        {
            isCall = true;
        }
    }
#endif
    return handle;
}

bool startHacking(JavaVM *vm)
{
    LOGI("Analysis tool begin");
    // MyUtil::setupSignalHandler(SIGSEGV);
    // DobbyHook((void*)dlopen, (dobby_dummy_func_t)newdlopen, (dobby_dummy_func_t*)&olddlopen);
    // int DobbyHook(void *address, dobby_dummy_func_t replace_func, dobby_dummy_func_t *origin_func);
    // by_pointer_t bySoHandle = by_dlopen("libc.so", BY_RTLD_LAZY);
    // DobbyHook((void*)(by_dlsym(bySoHandle, "open")), (dobby_dummy_func_t)newopen, (dobby_dummy_func_t*)&oldopen);
    void *do_dlopenAddr = DobbySymbolResolver("/system/bin/linker64", "__dl__Z9do_dlopenPKciPK17android_dlextinfoPKv");
    if (do_dlopenAddr)
    {
        LOGI("do_dlopenAddr:[%p]", do_dlopenAddr);
        DobbyHook(do_dlopenAddr, (dobby_dummy_func_t)newdo_dlopen, (dobby_dummy_func_t *)&olddo_dlopen);
    }
    // pthread_t mythread;
    // pthread_create(&mythread, nullptr, reinterpret_cast<void *(*)(void *)>(myCheckMap), nullptr);
    return true;
}