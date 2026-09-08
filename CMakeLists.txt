cmake_minimum_required(VERSION 3.21)

project(VirtualDesktop VERSION 1.0.0.1 LANGUAGES CXX RC)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

include(FetchContent)
set(WIL_BUILD_TESTS OFF)
set(WIL_BUILD_PACKAGING OFF)
FetchContent_Declare(WIL
    GIT_REPOSITORY https://github.com/microsoft/wil
    GIT_TAG        v1.0.260126.7
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(WIL)

set(SPDLOG_USE_STD_FORMAT ON)
set(SPDLOG_BUILD_EXAMPLE OFF)
set(SPDLOG_BUILD_TESTS OFF)
set(SPDLOG_INSTALL OFF)
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG        v1.17.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(spdlog)

enable_testing()

add_subdirectory(src/wilx)
add_subdirectory(src/app)
add_subdirectory(tests)
