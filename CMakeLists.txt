project(
    sync-cpp
    LANGUAGES CXX
    VERSION 0.2.0
    DESCRIPTION "Synchronized object wrapper for C++20"
)

add_library(sync_cpp INTERFACE)
target_include_directories(sync_cpp INTERFACE include)
set_target_properties(sync_cpp PROPERTIES CXX_EXTENSIONS OFF)
