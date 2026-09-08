set(WILX_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/../../assets/app.ico")
configure_file(app.rc.in app.rc @ONLY)

add_executable(VirtualDesktop WIN32
    main.cpp
    dialog.cpp
    logging.cpp
    dialog.h
    logging.h
    CommonDef.h
    resource.h
    stdafx.h
    "${CMAKE_CURRENT_BINARY_DIR}/app.rc"
)
target_include_directories(VirtualDesktop PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(VirtualDesktop PRIVATE wilx spdlog::spdlog)
target_precompile_headers(VirtualDesktop PRIVATE stdafx.h)
target_compile_features(VirtualDesktop PRIVATE cxx_std_23)
target_compile_definitions(VirtualDesktop PRIVATE UNICODE _UNICODE _WINDOWS)
target_compile_options(VirtualDesktop PRIVATE /W4)
set_target_properties(VirtualDesktop PROPERTIES
    OUTPUT_NAME "Virtual Desktop"
    VS_GLOBAL_KEYWORD "Win32Proj"
)
set_property(TARGET VirtualDesktop PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
