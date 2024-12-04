cmake_minimum_required(VERSION 3.4.1)
project(MyHook)

#add_definitions("-Wall") 没用
#add_compile_options("-fvisibility=hidden") # 加了后，会隐藏导出符号
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")

# 设置Lua脚本的路径
set(HACK_SCRIPT_PATH "${CMAKE_SOURCE_DIR}/LuaScript/hack.lua")
set(B64_HACK_SCRIPT_PATH "${CMAKE_SOURCE_DIR}/LuaScript/hack.lua.b64")

# 将Lua脚本编码为Base64
add_custom_command(
        OUTPUT ${B64_HACK_SCRIPT_PATH}
        COMMAND base64 -w 0 ${HACK_SCRIPT_PATH} > ${B64_HACK_SCRIPT_PATH}
        DEPENDS ${HACK_SCRIPT_PATH}
        COMMENT "Encoding ${HACK_SCRIPT_PATH} to Base64 without line breaks"
)

# 自定义目标，生成C++头文件
add_custom_target(generate_hack_lua_header ALL
    DEPENDS ${B64_HACK_SCRIPT_PATH}
    COMMAND ${CMAKE_COMMAND} -E echo "Generating C++ header file"
    COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Reading Base64 encoded file: ${B64_HACK_SCRIPT_PATH}"
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/generate_header.cmake
)

# 创建CMake脚本来生成头文件
file(WRITE "${CMAKE_BINARY_DIR}/generate_header.cmake" "
file(READ \"${B64_HACK_SCRIPT_PATH}\" B64_LUA_SCRIPT_CONTENTS)
string(REPLACE \"\\n\" \"\" B64_LUA_SCRIPT_CONTENTS \"\${B64_LUA_SCRIPT_CONTENTS}\")
file(WRITE \"${CMAKE_SOURCE_DIR}/LuaScript/hack.lua.b64.h\" \"const char* b64HackLua = \\\"\${B64_LUA_SCRIPT_CONTENTS}\\\";\\n\")
")

# 设置Lua脚本的路径
set(setGTable_SCRIPT_PATH "${CMAKE_SOURCE_DIR}/LuaScript/setGTable.lua")
set(B64_setGTable_SCRIPT_PATH "${CMAKE_SOURCE_DIR}/LuaScript/setGTable.lua.b64")

# 将Lua脚本编码为Base64
add_custom_command(
        OUTPUT ${B64_setGTable_SCRIPT_PATH}
        COMMAND base64 -w 0 ${setGTable_SCRIPT_PATH} > ${B64_setGTable_SCRIPT_PATH}
        DEPENDS ${setGTable_SCRIPT_PATH}
        COMMENT "Encoding ${setGTable_SCRIPT_PATH} to Base64 without line breaks"
)

# 自定义目标，生成C++头文件
add_custom_target(generate_setGTable_lua_header ALL
    DEPENDS ${B64_setGTable_SCRIPT_PATH}
    COMMAND ${CMAKE_COMMAND} -E echo "Generating C++ header file"
    COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Reading Base64 encoded file: ${B64_setGTable_SCRIPT_PATH}"
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/generate_setGTable_lua_header.cmake
)

# 创建CMake脚本来生成头文件
file(WRITE "${CMAKE_BINARY_DIR}/generate_setGTable_lua_header.cmake" "
file(READ \"${B64_setGTable_SCRIPT_PATH}\" B64_setGTable_SCRIPT_CONTENT)
string(REPLACE \"\\n\" \"\" B64_setGTable_SCRIPT_CONTENT \"\${B64_setGTable_SCRIPT_CONTENT}\")
file(WRITE \"${CMAKE_SOURCE_DIR}/LuaScript/setGTable.lua.b64.h\" \"const char* b64setGTableLua = \\\"\${B64_setGTable_SCRIPT_CONTENT}\\\";\\n\")
")

aux_source_directory(framework/byOpen myByOpen)
aux_source_directory(framework/base64 myBase64)
aux_source_directory(framework/xhook myXhook)
aux_source_directory(framework/PrintStack myPrintStack)
aux_source_directory(framework/And64InlineHook myAnd64)
# aux_source_directory(framework/Zygisk-Il2CppDumper myZygisk-Il2CppDumper)
aux_source_directory(framework/Utils myUtils)
aux_source_directory(framework/NativeSandBox myNativeSandBox)

include_directories(framework/byOpen)
include_directories(framework/base64)
include_directories(framework/And64InlineHook)
include_directories(framework/xhook)
include_directories(framework/Dobby/include)
include_directories(framework/SSL/include)
include_directories(framework/PrintStack)
include_directories(framework/Zygisk-Il2CppDumper)
include_directories(framework/Utils/include)
include_directories(framework/NativeSandBox/include)

include_directories(.)

add_library(
        MyHook
        # Sets the library as a shared library.
        SHARED
        # **************************************************************************
        # Provides a relative path to your source file(s).
        #myTestCase.cpp
        #myUnityIL2Cpp.cpp
        #myXhook.cpp
        #myTimehook.cpp
        #myLua.cpp
        myAnalysis.cpp
        #myDobbyHttps.cpp
        # **************************************************************************
        ${myByOpen}
        ${myBase64}
        ${myAnd64}
        ${myXhook}
        ${myPrintStack}
        ${myUtils}
        # ${myZygisk-Il2CppDumper}
        ${myNativeSandBox}
        )

add_library(myDobby STATIC IMPORTED)
add_library(myNativeSandBox SHARED IMPORTED)

set_target_properties(myDobby PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/framework/Dobby/build/android/${ANDROID_ABI}/libdobby.a)
set_target_properties(myNativeSandBox PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/framework/NativeSandBox/cpp/build64/libMyNativeSandBox.so)

target_link_libraries(
        MyHook
        log
        myDobby
        myNativeSandBox
        )