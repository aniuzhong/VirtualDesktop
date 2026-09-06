cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED CMAKE_GENERATOR_TOOLSET AND CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_GENERATOR_TOOLSET "host=x64,version=14.44" CACHE STRING "MSVC toolset version" FORCE)
endif()

set(CMAKE_MFC_FLAG 1)

project(VirtualDesktop LANGUAGES CXX RC)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(VirtualDesktop WIN32
    DesktopManager.cpp
    GlobalFunctions.cpp
    RegSettings.cpp
    "Virtual Desktop.cpp"
    "Virtual DesktopDlg.cpp"
    stdafx.cpp
    "Virtual Desktop.rc"
    "res/Virtual Desktop.ico"
)
target_include_directories(VirtualDesktop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(VirtualDesktop PRIVATE stdafx.h)
target_compile_definitions(VirtualDesktop PRIVATE UNICODE _UNICODE _WINDOWS)
set_target_properties(VirtualDesktop PROPERTIES
    OUTPUT_NAME "Virtual Desktop"
    VS_GLOBAL_KEYWORD "MFCProj"
)

set_property(TARGET VirtualDesktop PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

add_library(EventHookerDll SHARED
    "Event Hooker Dll/Event Hooker Dll.cpp"
    "Event Hooker Dll/Global Functions.cpp"
    "Event Hooker Dll/stdafx.cpp"
    "Event Hooker Dll/Event Hooker Dll.rc"
)
target_include_directories(EventHookerDll PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/"Event Hooker Dll")
target_precompile_headers(EventHookerDll PRIVATE "Event Hooker Dll/stdafx.h")
target_compile_definitions(EventHookerDll PRIVATE UNICODE _UNICODE _WINDOWS _USRDLL)
set_target_properties(EventHookerDll PROPERTIES
    OUTPUT_NAME "Event Hooker Dll"
)
set_property(TARGET EventHookerDll PROPERTY LINK_FLAGS
    "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/Event Hooker Dll/Event Hooker Dll.def\"")
set_property(TARGET EventHookerDll PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

add_dependencies(VirtualDesktop EventHookerDll)
