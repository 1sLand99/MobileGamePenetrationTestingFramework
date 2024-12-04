#ifndef RIRU_IL2CPPDUMPER_IL2CPP_H
#define RIRU_IL2CPPDUMPER_IL2CPP_H

#include <HwBreakpointManager.h>
#include <Util.h>
#include <unistd.h>
#include "il2cpp_dump.h"
#include <dlfcn.h>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <Log.h>
#include <byopen.h>
#include "il2cpp-tabledefs.h"
#include "il2cpp-class.h"
#include <dobby.h>
#include <NativeSandBox.h>

#define DO_API(r, n, p) r(*n) p

#include "il2cpp-api-functions.h"

#undef DO_API

static void *il2cpp_handle = nullptr;
static uint64_t il2cpp_base = 0;

void il2cpp_dump(void *handle, char *outDir);
void il2cpp_basedump(size_t baseAddr, char *outDir);
void il2cpp_speed(void *handle);
void il2cppTrace(void *handle);

void init_il2cpp_api(){

#define DO_API(r, n, p)                   \
    n = (r(*) p)dlsym(il2cpp_handle, #n); \
    LOGI("Function %s loaded at address %p\n", #n, n);
// hexdump((size_t)n, 0x10);
// n = (r(*) p)DobbySymbolResolver("libil2cpp.so", #n); \

#include "il2cpp-api-functions.h"

#undef DO_API
}

uint64_t get_module_base(const char *module_name)
{
    uint64_t addr = 0;
    char line[1024];
    uint64_t start = 0;
    uint64_t end = 0;
    char flags[5];
    char path[PATH_MAX];

    FILE *fp = fopen("/proc/self/maps", "r");
    if (fp != nullptr)
    {
        while (fgets(line, sizeof(line), fp))
        {
            strcpy(path, "");
            sscanf(line, "%" PRIx64 "-%" PRIx64 " %s %*" PRIx64 " %*x:%*x %*u %s\n", &start, &end,
                   flags, path);
#if defined(__aarch64__)
            if (strstr(flags, "x") == 0) // TODO
                continue;
#endif
            if (strstr(path, module_name))
            {
                addr = start;
                break;
            }
        }
        fclose(fp);
    }
    return addr;
}

std::string get_method_modifier(uint32_t flags)
{
    std::stringstream outPut;
    auto access = flags & METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK;
    switch (access)
    {
    case METHOD_ATTRIBUTE_PRIVATE:
        outPut << "private ";
        break;
    case METHOD_ATTRIBUTE_PUBLIC:
        outPut << "public ";
        break;
    case METHOD_ATTRIBUTE_FAMILY:
        outPut << "protected ";
        break;
    case METHOD_ATTRIBUTE_ASSEM:
    case METHOD_ATTRIBUTE_FAM_AND_ASSEM:
        outPut << "internal ";
        break;
    case METHOD_ATTRIBUTE_FAM_OR_ASSEM:
        outPut << "protected internal ";
        break;
    }
    if (flags & METHOD_ATTRIBUTE_STATIC)
    {
        outPut << "static ";
    }
    if (flags & METHOD_ATTRIBUTE_ABSTRACT)
    {
        outPut << "abstract ";
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT)
        {
            outPut << "override ";
        }
    }
    else if (flags & METHOD_ATTRIBUTE_FINAL)
    {
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT)
        {
            outPut << "sealed override ";
        }
    }
    else if (flags & METHOD_ATTRIBUTE_VIRTUAL)
    {
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_NEW_SLOT)
        {
            outPut << "virtual ";
        }
        else
        {
            outPut << "override ";
        }
    }
    if (flags & METHOD_ATTRIBUTE_PINVOKE_IMPL)
    {
        outPut << "extern ";
    }
    return outPut.str();
}

bool _il2cpp_type_is_byref(const Il2CppType *type)
{
    auto byref = type->byref;
    if (il2cpp_type_is_byref)
    {
        byref = il2cpp_type_is_byref(type);
    }
    return byref;
}

std::string dump_method(Il2CppClass *klass)
{
    std::stringstream outPut;
    outPut << "\n\t// Methods\n";
    void *iter = nullptr;
    while (auto method = il2cpp_class_get_methods(klass, &iter))
    {
        // TODO attribute
        if (method->methodPointer)
        {
            outPut << "\t// RVA: 0x";
            outPut << std::hex << (uint64_t)method->methodPointer - il2cpp_base;
            outPut << " VA: 0x";
            outPut << std::hex << (uint64_t)method->methodPointer;
        }
        else
        {
            outPut << "\t// RVA: 0x VA: 0x0";
        }
        /*if (method->slot != 65535) {
            outPut << " Slot: " << std::dec << method->slot;
        }*/
        outPut << "\n\t";
        uint32_t iflags = 0;
        auto flags = il2cpp_method_get_flags(method, &iflags);
        outPut << get_method_modifier(flags);
        // TODO genericContainerIndex
        auto return_type = il2cpp_method_get_return_type(method);
        if (_il2cpp_type_is_byref(return_type))
        {
            outPut << "ref ";
        }
        auto return_class = il2cpp_class_from_type(return_type);
        outPut << il2cpp_class_get_name(return_class) << " " << il2cpp_method_get_name(method)
               << "(";
        auto param_count = il2cpp_method_get_param_count(method);
        for (int i = 0; i < param_count; ++i)
        {
            auto param = il2cpp_method_get_param(method, i);
            auto attrs = param->attrs;
            if (_il2cpp_type_is_byref(param))
            {
                if (attrs & PARAM_ATTRIBUTE_OUT && !(attrs & PARAM_ATTRIBUTE_IN))
                {
                    outPut << "out ";
                }
                else if (attrs & PARAM_ATTRIBUTE_IN && !(attrs & PARAM_ATTRIBUTE_OUT))
                {
                    outPut << "in ";
                }
                else
                {
                    outPut << "ref ";
                }
            }
            else
            {
                if (attrs & PARAM_ATTRIBUTE_IN)
                {
                    outPut << "[In] ";
                }
                if (attrs & PARAM_ATTRIBUTE_OUT)
                {
                    outPut << "[Out] ";
                }
            }
            auto parameter_class = il2cpp_class_from_type(param);
            outPut << il2cpp_class_get_name(parameter_class) << " "
                   << il2cpp_method_get_param_name(method, i);
            outPut << ", ";
        }
        if (param_count > 0)
        {
            outPut.seekp(-2, outPut.cur);
        }
        outPut << ") { }\n";
        // TODO GenericInstMethod
    }
    return outPut.str();
}

std::string dump_property(Il2CppClass *klass)
{
    std::stringstream outPut;
    outPut << "\n\t// Properties\n";
    void *iter = nullptr;
    while (auto prop_const = il2cpp_class_get_properties(klass, &iter))
    {
        // TODO attribute
        auto prop = const_cast<PropertyInfo *>(prop_const);
        auto get = il2cpp_property_get_get_method(prop);
        auto set = il2cpp_property_get_set_method(prop);
        auto prop_name = il2cpp_property_get_name(prop);
        outPut << "\t";
        Il2CppClass *prop_class = nullptr;
        uint32_t iflags = 0;
        if (get)
        {
            outPut << get_method_modifier(il2cpp_method_get_flags(get, &iflags));
            prop_class = il2cpp_class_from_type(il2cpp_method_get_return_type(get));
        }
        else if (set)
        {
            outPut << get_method_modifier(il2cpp_method_get_flags(set, &iflags));
            auto param = il2cpp_method_get_param(set, 0);
            prop_class = il2cpp_class_from_type(param);
        }
        if (prop_class)
        {
            outPut << il2cpp_class_get_name(prop_class) << " " << prop_name << " { ";
            if (get)
            {
                outPut << "get; ";
            }
            if (set)
            {
                outPut << "set; ";
            }
            outPut << "}\n";
        }
        else
        {
            if (prop_name)
            {
                outPut << " // unknown property " << prop_name;
            }
        }
    }
    return outPut.str();
}

std::string dump_field(Il2CppClass *klass)
{
    std::stringstream outPut;
    outPut << "\n\t// Fields\n";
    LOGI("il2cpp_class_is_enum");
    auto is_enum = il2cpp_class_is_enum(klass);
    void *iter = nullptr;
    LOGI("il2cpp_class_get_fields");
    while (auto field = il2cpp_class_get_fields(klass, &iter))
    {
        // TODO attribute
        outPut << "\t";
        auto attrs = il2cpp_field_get_flags(field);
        auto access = attrs & FIELD_ATTRIBUTE_FIELD_ACCESS_MASK;
        switch (access)
        {
        case FIELD_ATTRIBUTE_PRIVATE:
            outPut << "private ";
            break;
        case FIELD_ATTRIBUTE_PUBLIC:
            outPut << "public ";
            break;
        case FIELD_ATTRIBUTE_FAMILY:
            outPut << "protected ";
            break;
        case FIELD_ATTRIBUTE_ASSEMBLY:
        case FIELD_ATTRIBUTE_FAM_AND_ASSEM:
            outPut << "internal ";
            break;
        case FIELD_ATTRIBUTE_FAM_OR_ASSEM:
            outPut << "protected internal ";
            break;
        }
        if (attrs & FIELD_ATTRIBUTE_LITERAL)
        {
            outPut << "const ";
        }
        else
        {
            if (attrs & FIELD_ATTRIBUTE_STATIC)
            {
                outPut << "static ";
            }
            if (attrs & FIELD_ATTRIBUTE_INIT_ONLY)
            {
                outPut << "readonly ";
            }
        }
        auto field_type = il2cpp_field_get_type(field);
        auto field_class = il2cpp_class_from_type(field_type);
        outPut << il2cpp_class_get_name(field_class) << " " << il2cpp_field_get_name(field);
        // TODO 获取构造函数初始化后的字段值
        if (attrs & FIELD_ATTRIBUTE_LITERAL && is_enum)
        {
            uint64_t val = 0;
            il2cpp_field_static_get_value(field, &val);
            outPut << " = " << std::dec << val;
        }
        outPut << "; // 0x" << std::hex << il2cpp_field_get_offset(field) << "\n";
    }
    return outPut.str();
}

std::string basedump_type(const Il2CppType *type)
{
    std::stringstream outPut;
    LOGI("il2cpp_class_from_type");
    auto *klass = il2cpp_class_from_type(type);
    LOGI("il2cpp_class_get_namespace");
    outPut << "\n// Namespace: " << il2cpp_class_get_namespace(klass) << "\n";
    LOGI("il2cpp_class_get_flags");
    auto flags = il2cpp_class_get_flags(klass);
    if (flags & TYPE_ATTRIBUTE_SERIALIZABLE)
    {
        outPut << "[Serializable]\n";
    }
    LOGI("il2cpp_class_is_valuetype");
    // TODO attribute
    auto is_valuetype = il2cpp_class_is_valuetype(klass);
    LOGI("il2cpp_class_is_enum");
    auto is_enum = il2cpp_class_is_enum(klass);
    auto visibility = flags & TYPE_ATTRIBUTE_VISIBILITY_MASK;
    switch (visibility)
    {
    case TYPE_ATTRIBUTE_PUBLIC:
    case TYPE_ATTRIBUTE_NESTED_PUBLIC:
        outPut << "public ";
        break;
    case TYPE_ATTRIBUTE_NOT_PUBLIC:
    case TYPE_ATTRIBUTE_NESTED_FAM_AND_ASSEM:
    case TYPE_ATTRIBUTE_NESTED_ASSEMBLY:
        outPut << "internal ";
        break;
    case TYPE_ATTRIBUTE_NESTED_PRIVATE:
        outPut << "private ";
        break;
    case TYPE_ATTRIBUTE_NESTED_FAMILY:
        outPut << "protected ";
        break;
    case TYPE_ATTRIBUTE_NESTED_FAM_OR_ASSEM:
        outPut << "protected internal ";
        break;
    }
    if (flags & TYPE_ATTRIBUTE_ABSTRACT && flags & TYPE_ATTRIBUTE_SEALED)
    {
        outPut << "static ";
    }
    else if (!(flags & TYPE_ATTRIBUTE_INTERFACE) && flags & TYPE_ATTRIBUTE_ABSTRACT)
    {
        outPut << "abstract ";
    }
    else if (!is_valuetype && !is_enum && flags & TYPE_ATTRIBUTE_SEALED)
    {
        outPut << "sealed ";
    }
    if (flags & TYPE_ATTRIBUTE_INTERFACE)
    {
        outPut << "interface ";
    }
    else if (is_enum)
    {
        outPut << "enum ";
    }
    else if (is_valuetype)
    {
        outPut << "struct ";
    }
    else
    {
        outPut << "class ";
    }
    LOGI("il2cpp_class_get_name");
    outPut << il2cpp_class_get_name(klass); // TODO genericContainerIndex
    std::vector<std::string> extends;
    LOGI("il2cpp_class_get_parent");
    auto parent = il2cpp_class_get_parent(klass);
    if (!is_valuetype && !is_enum && parent)
    {
        LOGI("il2cpp_class_get_type");
        auto parent_type = il2cpp_class_get_type(parent);
        if (parent_type->type != IL2CPP_TYPE_OBJECT)
        {
            extends.emplace_back(il2cpp_class_get_name(parent));
        }
    }
    void *iter = nullptr;
    LOGI("il2cpp_class_get_interfaces");
    while (auto itf = il2cpp_class_get_interfaces(klass, &iter))
    {
        extends.emplace_back(il2cpp_class_get_name(itf));
    }
    if (!extends.empty())
    {
        outPut << " : " << extends[0];
        for (int i = 1; i < extends.size(); ++i)
        {
            outPut << ", " << extends[i];
        }
    }
    outPut << "\n{";
    LOGI("dump_field");
    outPut << dump_field(klass);
    outPut << dump_property(klass);
    LOGI("dump_method");
    outPut << dump_method(klass);
    // TODO EventInfo
    outPut << "}\n";
    return outPut.str();
}

std::string dump_type(const Il2CppType *type)
{
    std::stringstream outPut;
    auto *klass = il2cpp_class_from_type(type);
    outPut << "\n// Namespace: " << il2cpp_class_get_namespace(klass) << "\n";
    auto flags = il2cpp_class_get_flags(klass);
    if (flags & TYPE_ATTRIBUTE_SERIALIZABLE)
    {
        outPut << "[Serializable]\n";
    }
    // TODO attribute
    auto is_valuetype = il2cpp_class_is_valuetype(klass);
    auto is_enum = il2cpp_class_is_enum(klass);
    auto visibility = flags & TYPE_ATTRIBUTE_VISIBILITY_MASK;
    switch (visibility)
    {
    case TYPE_ATTRIBUTE_PUBLIC:
    case TYPE_ATTRIBUTE_NESTED_PUBLIC:
        outPut << "public ";
        break;
    case TYPE_ATTRIBUTE_NOT_PUBLIC:
    case TYPE_ATTRIBUTE_NESTED_FAM_AND_ASSEM:
    case TYPE_ATTRIBUTE_NESTED_ASSEMBLY:
        outPut << "internal ";
        break;
    case TYPE_ATTRIBUTE_NESTED_PRIVATE:
        outPut << "private ";
        break;
    case TYPE_ATTRIBUTE_NESTED_FAMILY:
        outPut << "protected ";
        break;
    case TYPE_ATTRIBUTE_NESTED_FAM_OR_ASSEM:
        outPut << "protected internal ";
        break;
    }
    if (flags & TYPE_ATTRIBUTE_ABSTRACT && flags & TYPE_ATTRIBUTE_SEALED)
    {
        outPut << "static ";
    }
    else if (!(flags & TYPE_ATTRIBUTE_INTERFACE) && flags & TYPE_ATTRIBUTE_ABSTRACT)
    {
        outPut << "abstract ";
    }
    else if (!is_valuetype && !is_enum && flags & TYPE_ATTRIBUTE_SEALED)
    {
        outPut << "sealed ";
    }
    if (flags & TYPE_ATTRIBUTE_INTERFACE)
    {
        outPut << "interface ";
    }
    else if (is_enum)
    {
        outPut << "enum ";
    }
    else if (is_valuetype)
    {
        outPut << "struct ";
    }
    else
    {
        outPut << "class ";
    }
    outPut << il2cpp_class_get_name(klass); // TODO genericContainerIndex
    std::vector<std::string> extends;
    auto parent = il2cpp_class_get_parent(klass);
    if (!is_valuetype && !is_enum && parent)
    {
        auto parent_type = il2cpp_class_get_type(parent);
        if (parent_type->type != IL2CPP_TYPE_OBJECT)
        {
            extends.emplace_back(il2cpp_class_get_name(parent));
        }
    }
    void *iter = nullptr;
    while (auto itf = il2cpp_class_get_interfaces(klass, &iter))
    {
        extends.emplace_back(il2cpp_class_get_name(itf));
    }
    if (!extends.empty())
    {
        outPut << " : " << extends[0];
        for (int i = 1; i < extends.size(); ++i)
        {
            outPut << ", " << extends[i];
        }
    }
    outPut << "\n{";
    outPut << dump_field(klass);
    outPut << dump_property(klass);
    outPut << dump_method(klass);
    // TODO EventInfo
    outPut << "}\n";
    return outPut.str();
}

void il2cpp_basedump(size_t baseAddr, char *outDir)
{
    // initialize
    LOGI("baseAddr: %p", baseAddr);
    il2cpp_base = baseAddr;
    LOGI("sleep finish in 5s");
    sleep(5);

    il2cpp_thread_attach = (Il2CppThread * (*)(Il2CppDomain * domain))(baseAddr + 0xdf6a2c);

    il2cpp_domain_get = (Il2CppDomain * (*)())(baseAddr + 0xE37A4C);
    il2cpp_domain_get_assemblies = (const Il2CppAssembly **(*)(const Il2CppDomain *domain, size_t *size))(baseAddr + 0xDF6780);

    il2cpp_assembly_get_image = (const Il2CppImage *(*)(const Il2CppAssembly *assembly))(baseAddr + 0xDF6494);

    il2cpp_image_get_name = (const char *(*)(const Il2CppImage *image))(baseAddr + 0xDF6B8C);
    il2cpp_image_get_class = (const Il2CppClass *(*)(const Il2CppImage *image, size_t index))(baseAddr + 0xdf6b9c);
    il2cpp_image_get_class_count = (size_t(*)(const Il2CppImage *image))(baseAddr + 0xdf6b98);

    il2cpp_class_get_type = (const Il2CppType *(*)(Il2CppClass *klass))(baseAddr + 0xdf647c);
    il2cpp_class_from_type = (Il2CppClass * (*)(const Il2CppType *type))(baseAddr + 0xdf6474);
    il2cpp_class_get_namespace = (const char *(*)(Il2CppClass *klass))(baseAddr + 0xdf6444);
    il2cpp_class_get_flags = (int (*)(const Il2CppClass *klass))(baseAddr + 0xdf6464);
    il2cpp_class_is_valuetype = (bool (*)(const Il2CppClass *klass))(baseAddr + 0xdf6458);
    il2cpp_class_is_enum = (bool (*)(const Il2CppClass *klass))(baseAddr + 0xdf6490);
    il2cpp_class_get_name = (const char *(*)(Il2CppClass *klass))(baseAddr + 0xdf6440);
    il2cpp_class_get_parent = (Il2CppClass * (*)(Il2CppClass * klass))(baseAddr + 0xdf6448);
    il2cpp_class_get_interfaces = (Il2CppClass * (*)(Il2CppClass * klass, void **iter))(baseAddr + 0xdf6428);
    il2cpp_class_get_fields = (FieldInfo * (*)(Il2CppClass * klass, void **iter))(baseAddr + 0xdf6420);
    il2cpp_class_get_methods = (const MethodInfo *(*)(Il2CppClass *klass, void **iter))(baseAddr + 0xdf6438);
    il2cpp_class_get_properties = (const PropertyInfo *(*)(Il2CppClass *klass, void **iter))(baseAddr + 0xdf642c);

    il2cpp_property_get_get_method = (const MethodInfo *(*)(PropertyInfo *prop))(baseAddr + 0xdf6964);
    il2cpp_property_get_set_method = (const MethodInfo *(*)(PropertyInfo *prop))(baseAddr + 0xdf6968);
    il2cpp_property_get_name = (const char *(*)(PropertyInfo *prop))(baseAddr + 0xdf6960);

    il2cpp_field_get_type = (const Il2CppType *(*)(FieldInfo *field))(baseAddr + 0xdf686c);
    il2cpp_field_get_name = (const char *(*)(FieldInfo *field))(baseAddr + 0xdf685c);
    il2cpp_field_static_get_value = (void (*)(FieldInfo *field, void *value))(baseAddr + 0xdf6884);
    il2cpp_field_get_offset = (size_t(*)(FieldInfo * field))(baseAddr + 0xdf6868);
    il2cpp_field_get_flags = (int (*)(FieldInfo *field))(baseAddr + 0xdf6860);

    il2cpp_method_get_flags = (uint32_t(*)(const MethodInfo *method, uint32_t *iflags))(baseAddr + 0xdf6908);
    il2cpp_method_get_return_type = (const Il2CppType *(*)(const MethodInfo *method))(baseAddr + 0xdf68d8);
    il2cpp_method_get_name = (const char *(*)(const MethodInfo *method))(baseAddr + 0xdf68e4);
    il2cpp_method_get_param_count = (uint32_t(*)(const MethodInfo *method))(baseAddr + 0xdf68f4);
    il2cpp_method_get_param = (const Il2CppType *(*)(const MethodInfo *method, uint32_t index))(baseAddr + 0xdf68f8);
    il2cpp_method_get_param_name = (const char *(*)(const MethodInfo *method, uint32_t index))(baseAddr + 0xdf6940);

    auto domain = il2cpp_domain_get();
    LOGI("domain:%p", domain);
    il2cpp_thread_attach(domain);
    // start dump
    LOGI("dumping...");
    size_t size;
    auto assemblies = il2cpp_domain_get_assemblies(domain, &size);
    std::stringstream imageOutput;
    std::vector<std::string> outPuts;
    if (il2cpp_image_get_class)
    {
        LOGI("Version greater than 2018.3");
        for (int i = 0; i < size; ++i)
        {
            LOGI("il2cpp_assembly_get_image");
            auto image = il2cpp_assembly_get_image(assemblies[i]);
            std::stringstream imageStr;
            LOGI("il2cpp_image_get_name");
            imageStr << "\n// Dll : " << il2cpp_image_get_name(image);
            LOGI("il2cpp_image_get_class_count");
            auto classCount = il2cpp_image_get_class_count(image);
            for (int j = 0; j < classCount; ++j)
            {
                LOGI("il2cpp_image_get_class");
                auto klass = il2cpp_image_get_class(image, j);
                LOGI("il2cpp_class_get_type");
                auto type = il2cpp_class_get_type(const_cast<Il2CppClass *>(klass));
                // LOGD("type name : %s", il2cpp_type_get_name(type));
                // auto outPut = imageStr.str() + basedump_type(type);
                auto outPut = imageStr.str() + dump_type(type);
                outPuts.push_back(outPut);
            }
        }
    }
    LOGI("write dump file");
    auto outPath = std::string(outDir).append("/files/dump.cs");
    std::ofstream outStream(outPath);
    outStream << imageOutput.str();
    auto count = outPuts.size();
    for (int i = 0; i < count; ++i)
    {
        outStream << outPuts[i];
    }
    outStream.close();
    LOGI("dump done, %s", outPath.c_str());
}

void il2cpp_dump(void *handle, char *outDir)
{
    // initialize
    LOGI("il2cpp_handle: %p", handle);
    il2cpp_handle = handle;
    init_il2cpp_api();
    LOGI("init_il2cpp_api done");
    if (il2cpp_domain_get_assemblies)
    {
        Dl_info dlInfo;
        if (dladdr((void *)il2cpp_domain_get_assemblies, &dlInfo))
        {
            il2cpp_base = reinterpret_cast<uint64_t>(dlInfo.dli_fbase);
        }
        else
        {
            LOGI("dladdr error, using get_module_base.");
            il2cpp_base = get_module_base("libil2cpp.so");
        }
        LOGI("il2cpp_base:[%p]", il2cpp_base);
    }
    else
    {
        LOGE("Failed to initialize il2cpp api.");
        LOGI("il2cpp_domain_get:%p il2cpp_image_get_class:%p il2cpp_class_from_name:%p", il2cpp_domain_get, il2cpp_image_get_class, il2cpp_class_from_name);
        return;
    }
    LOGI("il2cpp_domain_get:%p il2cpp_thread_attach:%p", (size_t)((size_t)il2cpp_domain_get - il2cpp_base), (size_t)((size_t)il2cpp_thread_attach - il2cpp_base));
    auto domain = il2cpp_domain_get();
    LOGI("domain:%p", domain);
    il2cpp_thread_attach(domain);
    LOGI("il2cpp thread attach success");
    // start dump
    LOGI("dumping...");
    size_t size;
    auto assemblies = il2cpp_domain_get_assemblies(domain, &size);
    std::stringstream imageOutput;
    for (int i = 0; i < size; ++i)
    {
        auto image = il2cpp_assembly_get_image(assemblies[i]);
        imageOutput << "// Image " << i << ": " << il2cpp_image_get_name(image) << "\n";
    }
    std::vector<std::string> outPuts;
    if (il2cpp_image_get_class)
    {
        LOGI("Version greater than 2018.3");
        // 使用il2cpp_image_get_class
        for (int i = 0; i < size; ++i)
        {
            auto image = il2cpp_assembly_get_image(assemblies[i]);
            std::stringstream imageStr;
            imageStr << "\n// Dll : " << il2cpp_image_get_name(image);
            auto classCount = il2cpp_image_get_class_count(image);
            for (int j = 0; j < classCount; ++j)
            {
                auto klass = il2cpp_image_get_class(image, j);
                auto type = il2cpp_class_get_type(const_cast<Il2CppClass *>(klass));
                // LOGD("type name : %s", il2cpp_type_get_name(type));
                auto outPut = imageStr.str() + dump_type(type);
                outPuts.push_back(outPut);
            }
        }
    }
    else
    {
        LOGI("Version less than 2018.3");
        // 使用反射
        auto corlib = il2cpp_get_corlib();
        auto assemblyClass = il2cpp_class_from_name(corlib, "System.Reflection", "Assembly");
        auto assemblyLoad = il2cpp_class_get_method_from_name(assemblyClass, "Load", 1);
        auto assemblyGetTypes = il2cpp_class_get_method_from_name(assemblyClass, "GetTypes", 0);
        if (assemblyLoad && assemblyLoad->methodPointer)
        {
            LOGI("Assembly::Load: %p", assemblyLoad->methodPointer);
        }
        else
        {
            LOGI("miss Assembly::Load");
            return;
        }
        if (assemblyGetTypes && assemblyGetTypes->methodPointer)
        {
            LOGI("Assembly::GetTypes: %p", assemblyGetTypes->methodPointer);
        }
        else
        {
            LOGI("miss Assembly::GetTypes");
            return;
        }
        typedef void *(*Assembly_Load_ftn)(void *, Il2CppString *, void *);
        typedef Il2CppArray *(*Assembly_GetTypes_ftn)(void *, void *);
        for (int i = 0; i < size; ++i)
        {
            auto image = il2cpp_assembly_get_image(assemblies[i]);
            std::stringstream imageStr;
            auto image_name = il2cpp_image_get_name(image);
            imageStr << "\n// Dll : " << image_name;
            // LOGD("image name : %s", image->name);
            auto imageName = std::string(image_name);
            auto pos = imageName.rfind('.');
            auto imageNameNoExt = imageName.substr(0, pos);
            auto assemblyFileName = il2cpp_string_new(imageNameNoExt.c_str());
            auto reflectionAssembly = ((Assembly_Load_ftn)assemblyLoad->methodPointer)(nullptr,
                                                                                       assemblyFileName,
                                                                                       nullptr);
            auto reflectionTypes = ((Assembly_GetTypes_ftn)assemblyGetTypes->methodPointer)(
                reflectionAssembly, nullptr);
            auto items = reflectionTypes->vector;
            for (int j = 0; j < reflectionTypes->max_length; ++j)
            {
                auto klass = il2cpp_class_from_system_type((Il2CppReflectionType *)items[j]);
                auto type = il2cpp_class_get_type(klass);
                // LOGD("type name : %s", il2cpp_type_get_name(type));
                auto outPut = imageStr.str() + dump_type(type);
                outPuts.push_back(outPut);
            }
        }
    }
    LOGI("write dump file");
    auto outPath = std::string(outDir).append("/files/dump.cs");
    std::ofstream outStream(outPath);
    outStream << imageOutput.str();
    auto count = outPuts.size();
    for (int i = 0; i < count; ++i)
    {
        outStream << outPuts[i];
    }
    outStream.close();
    LOGI("dump done, %s", outPath.c_str());
}

// public static Void set_timeScale(Single value) { }
void *(*oldset_timeScale)(float f);
void *newset_timeScale(float f)
{
    LOGI("timescale f:%f", f);
    f = MySpeed;
    return oldset_timeScale(f);
}

// Il2CppObject* il2cpp_runtime_invoke(const MethodInfo *method, void *obj, void **params, Il2CppException **exc)
typedef const char *(*typeil2cpp_method_get_name)(const MethodInfo *method);
typeil2cpp_method_get_name il2cpp_method_get_name_addr;

void il2cpp_runtime_invokeCB(void *address, DobbyRegisterContext *ctx)
{
    LOGI("il2cpp_runtime_invoke");
    if (ctx->general.regs.x0)
    {
        const char *methodName = il2cpp_method_get_name_addr((MethodInfo *)ctx->general.regs.x0);
        if (methodName)
        {
            LOGI("methodName:%s", methodName);
        }
    }
    return;
}

void il2cppTrace(void *handle)
{
    void *il2cpp_runtime_invoke_addr = dlsym(handle, "il2cpp_runtime_invoke");
    LOGI("il2cpp: [%p] [%p]", handle, il2cpp_runtime_invoke_addr);
    Dl_info info = MyUtil::getAddrDLInfo(il2cpp_runtime_invoke_addr);
    il2cpp_method_get_name_addr = (typeil2cpp_method_get_name)DobbySymbolResolver(info.dli_fname, "il2cpp_method_get_name");
    DobbyInstrument((void *)((size_t)info.dli_fbase + 0x136CA7C), il2cpp_runtime_invokeCB);
    // DobbyHook((void *)((size_t)info.dli_fbase + 0x136CA7C), (dobby_dummy_func_t)newil2cpp_runtime_invoke, (dobby_dummy_func_t *)&oldil2cpp_runtime_invoke);
    return;
}

void il2cpp_speed(void *handle)
{
    sleep(5);
    // initialize
    LOGI("il2cpp_handle: %p", handle);
    il2cpp_handle = handle;
    init_il2cpp_api();
    if (il2cpp_domain_get_assemblies)
    {
        Dl_info dlInfo;
        if (dladdr((void *)il2cpp_domain_get_assemblies, &dlInfo))
        {
            il2cpp_base = reinterpret_cast<uint64_t>(dlInfo.dli_fbase);
        }
        else
        {
            LOGW("dladdr error, using get_module_base.");
            il2cpp_base = get_module_base("libil2cpp.so");
        }
        LOGI("il2cpp_base: %" PRIx64 "", il2cpp_base);
    }
    else
    {
        LOGE("Failed to initialize il2cpp api.");
        return;
    }
    LOGI("il2cpp_domain_get:%zx il2cpp_thread_attach:%zx", (size_t)((size_t)il2cpp_domain_get - il2cpp_base), (size_t)((size_t)il2cpp_thread_attach - il2cpp_base));
    auto domain = il2cpp_domain_get();
    il2cpp_thread_attach(domain);
    size_t size;
    Il2CppAssembly *unityCore = (Il2CppAssembly *)il2cpp_domain_assembly_open(domain, "UnityEngine.CoreModule");
    Il2CppImage *unityImage = (Il2CppImage *)il2cpp_assembly_get_image(unityCore);
    Il2CppClass *timeClass = il2cpp_class_from_name((const Il2CppImage *)unityImage, "UnityEngine", "Time");
    MethodInfo *setTimeScaleMethodInfo = (MethodInfo *)il2cpp_class_get_method_from_name(timeClass, "set_timeScale", 1);
    LOGI("setTimeScaleMethodInfo:%zx", (size_t)((size_t)setTimeScaleMethodInfo->methodPointer - il2cpp_base));
    DobbyHook((void *)setTimeScaleMethodInfo->methodPointer, (dobby_dummy_func_t)newset_timeScale, (dobby_dummy_func_t *)&oldset_timeScale);
    return;
}

#endif // RIRU_IL2CPPDUMPER_IL2CPP_H