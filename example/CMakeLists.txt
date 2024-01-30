cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COLOR_DIAGNOSTICS ON)

project(sync-cpp-example)

add_subdirectory(./lib/sync-cpp)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE sync_cpp)

# thread sanitizer
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(main PRIVATE /fsanitize=thread)
    target_link_options(main PRIVATE /fsanitize=thread)
else()
    target_compile_options(main PRIVATE -fsanitize=thread)
    target_link_options(main PRIVATE -fsanitize=thread)
endif()
