add_library(wilx INTERFACE)
target_compile_features(wilx INTERFACE cxx_std_23)
target_include_directories(wilx INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(wilx INTERFACE WIL::WIL)
