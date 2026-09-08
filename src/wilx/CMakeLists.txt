add_library(wilx INTERFACE)
target_compile_features(wilx INTERFACE cxx_std_23)
# Headers live flat in src/wilx; src/ is the include root so callers keep
# including <wilx/...>.
target_include_directories(wilx INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(wilx INTERFACE WIL::WIL)
