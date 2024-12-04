#include <jni.h>
#include <string>
#include <dlfcn.h>
#include <pthread.h>
#include <android/log.h>
#include <unistd.h>
#include <thread>

#include "framework/Substrate/CydiaSubstrate.h"
#include "framework/And64InlineHook/And64InlineHook.hpp"
#include "framework/base64/base64.h"

#include "mono2.h"

#define LOG_TAG   "mylib"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__)

//cd /root/Desktop/MySo/app/build/intermediates/cmake/release/obj/armeabi-v7a/

std::string soName = "libmonobdwgc-2.0.so";

static bool print_stack_frame_to_logcat(StackFrameInfo * frame, void *ctx, void * data);

static bool module_Is_Load = false;

static void (*mono_walk_stack)(void * callBack, MonoUnwindOptions options, void *user_data);

static MonoMethod *(*mono_jit_info_get_method)(MonoJitInfo * jit);

static void *(*mono_domain_get)(void);

static char * (*mono_method_full_name)(MonoMethod * mono_method, bool signature);

static void * (*mono_compile_method)(MonoMethod *method);

static void * g_MonoMethod = NULL;

static void (*mono_jit_info_table_add)(void * domain, MonoJitInfo * jit);
static void (*mono_jit_info_table_add_f)(void * domain, MonoJitInfo * jit);
static void mono_jit_info_table_add_hook(void * domain, MonoJitInfo * jit)
{
    if(jit->d.method && jit->d.method->name && jit->d.method->klass && jit->d.method->klass->name){
        MonoMethod * mono_method = (MonoMethod *)mono_jit_info_get_method(jit);
        char * mono_method_name = mono_method_full_name(mono_method, true);
        //LOGD("mono_method_name : %s", mono_method_name);
        if(strcasestr(mono_method_name, "(wrapper native-to-managed) XLua.CSObjectWrap.UnityEngineUIGridLayoutGroupWrap:_g_get_cellSize (intptr)"))
        {
            g_MonoMethod = mono_method;
            LOGD("mono_method_name : %s", mono_method_name);
            LOGD("stack trace 1");
            mono_walk_stack((void*)print_stack_frame_to_logcat, MonoUnwindOptions::MONO_UNWIND_LOOKUP_ALL, NULL);
            LOGD("stack trace 2");
        }
    }
    return mono_jit_info_table_add_f(domain, jit);
}

static char *(*mono_debug_print_stack_frame)(void * MonoMethod, uint32_t native_offset, void * MonoDomain);

static bool print_stack_frame_to_logcat (StackFrameInfo * frame, void *ctx, void * data)
{
    MonoMethod *method = NULL;
    if (frame->ji && frame->type != FRAME_TYPE_TRAMPOLINE)
        method = mono_jit_info_get_method(frame->ji);
    if (method) {
        char *location = mono_debug_print_stack_frame (method, frame->native_offset, frame->domain);
        LOGD("  %s", location);
    } else
        LOGD("  at <unknown> <0x%05x>", frame->native_offset);
    return false;
}

static void myCheckMap(void)
{
    LOGD("checking Map");
    size_t pid = getpid();
    LOGD("pid : %d", pid);
    char line[512] = {0};
    char fileName[512] = {0};
    sprintf(fileName, "/proc/%d/maps", pid);
    LOGD("fileName : %s", fileName);
    size_t base_addr;
    size_t end_addr;
    char permission[512] = {0};
    size_t offset = 0;
    char fd[512] = {0};
    size_t softLink = 0;
    char path[512] = {0};
    LOGD("enter loop");
    FILE * fp = fopen(fileName, "r");
    while(1)
    {
        char * isEnd = fgets(line, 512, fp);
        if(isEnd == nullptr)
        {
            fseek(fp, 0, 0);
        }
        if(strstr(line, soName.c_str()))
        {
            if(offset == NULL)
            {
                if(strcasestr(permission, "r-xp"))
                {
                    LOGI("line : %s", line);
                    sscanf(line, "%zx-%zx %s %zx %s %d %s", &base_addr, &end_addr, permission, &offset, fd, &softLink, path);
                    LOGI("Library is Found! %s", path);
                    fclose(fp);
                    break;
                }   
            }
        }
    }
    void * soHandle = dlopen(soName.c_str(), RTLD_LAZY);
    LOGD("soHandle : %p", soHandle);
    if(soHandle)
    {
        LOGD("soHandle success, base_addr : %p", base_addr);
        //static void (*mono_walk_stack)(void * callBack, MonoUnwindOptions options, void *user_data);
        mono_walk_stack = (void (*) (void *, MonoUnwindOptions, void *))dlsym(soHandle, "mono_walk_stack");
        LOGD("mono_walk_stack : %p offset : %p", mono_walk_stack, (char*)mono_walk_stack - base_addr);

        mono_jit_info_get_method = (MonoMethod * (*) (MonoJitInfo*))dlsym(soHandle, "mono_jit_info_get_method");
        LOGD("mono_jit_info_get_method : %p  offset : %p", mono_jit_info_get_method, (char*)mono_jit_info_get_method - base_addr);

        mono_domain_get = (void * (*) (void))dlsym(soHandle, "mono_domain_get");
        LOGD("mono_domain_get : %p  offset : %p", mono_domain_get, (char*)mono_domain_get - base_addr);

        mono_debug_print_stack_frame = (char * (*) (void *, uint32_t, void *))dlsym(soHandle, "mono_debug_print_stack_frame");
        LOGD("mono_debug_print_stack_frame : %p  offset : %p", mono_debug_print_stack_frame, (char*)mono_debug_print_stack_frame - base_addr);

        mono_method_full_name = (char * (*) (MonoMethod *, bool))dlsym(soHandle, "mono_method_full_name");
        LOGD("mono_method_full_name : %p   offset : %p", mono_method_full_name, (char*)mono_method_full_name - base_addr);

        mono_jit_info_table_add = (void (*) (void *, MonoJitInfo *))dlsym(soHandle, "mono_jit_info_table_add");
        LOGD("mono_jit_info_table_add : %p  offset : %p", mono_jit_info_table_add, (char*)mono_jit_info_table_add - base_addr);

        mono_compile_method = (void* (*) (MonoMethod *))dlsym(soHandle, "mono_compile_method");
        LOGD("mono_compile_method : %p  offset : %p ", mono_compile_method, (char*)mono_compile_method - base_addr);

        MSHookFunction((void*)mono_jit_info_table_add, (void*)mono_jit_info_table_add_hook, (void**)&mono_jit_info_table_add_f);
        LOGD("mono_jit_info_table_add_hook : %p   mono_jit_info_table_add_f : %p", mono_jit_info_table_add_hook, mono_jit_info_table_add_f);
    }
}

extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* vm, void* reserved)
{
    LOGD("enter JNI_OnLoad!");
    pthread_t mythread;
    pthread_create(&mythread, nullptr, reinterpret_cast<void *(*)(void *)>(myCheckMap), nullptr);
    return JNI_VERSION_1_6;
    //return JNI_ERR;
}