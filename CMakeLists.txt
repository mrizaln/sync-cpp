cmake_minimum_required(VERSION 3.12)

project(
  sync-cpp
  LANGUAGES CXX
  VERSION 0.5.0
  DESCRIPTION "Synchronized object wrapper for C++20"
)

string(
  COMPARE EQUAL
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}"
  SYNC_CPP_STANDALONE
)

option(SYNC_CPP_BUILD_EXAMPLES "Build examples" ${SYNC_CPP_STANDALONE})
option(SYNC_CPP_BUILD_TESTS "Build tests" ${SYNC_CPP_STANDALONE})

add_library(sync-cpp INTERFACE)
target_include_directories(sync-cpp INTERFACE include)
target_compile_features(sync-cpp INTERFACE cxx_std_20)
set_target_properties(sync-cpp PROPERTIES CXX_EXTENSIONS OFF)

if(SYNC_CPP_BUILD_TESTS)
  add_subdirectory(test)
endif()

if(SYNC_CPP_BUILD_EXAMPLES)
  add_subdirectory(example)
endif()
