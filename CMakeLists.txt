project(
  sync-cpp
  LANGUAGES CXX
  VERSION 0.5.0
  DESCRIPTION "Synchronized object wrapper for C++20")

add_library(sync-cpp INTERFACE)
target_include_directories(sync-cpp INTERFACE include)
target_compile_features(sync-cpp INTERFACE cxx_std_20)
set_target_properties(sync-cpp PROPERTIES CXX_EXTENSIONS OFF)
