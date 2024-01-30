cmake_minimum_required(VERSION 3.12)

project(
    sync-cpp
    LANGUAGES CXX
    VERSION 0.1.0
    DESCRIPTION "Synchronized object wrapper for C++20"
)

add_library(sync_cpp INTERFACE)
target_include_directories(sync_cpp INTERFACE include)
set_target_properties(sync_cpp PROPERTIES CXX_EXTENSIONS OFF)

set(BUILD_TESTS OFF CACHE BOOL "Build tests")

if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(boost_ut
        GIT_REPOSITORY https://github.com/boost-ext/ut/
        GIT_TAG        v2.0.0
    )

    FetchContent_MakeAvailable(boost_ut)

    add_executable(sync_test test/sync_test.cpp)
    target_link_libraries(sync_test PRIVATE sync_cpp Boost::ut)

    add_executable(sync_container_test test/sync_container_test.cpp)
    target_link_libraries(sync_container_test PRIVATE sync_cpp Boost::ut)

    add_executable(sync_smart_ptr_test test/sync_smart_ptr_test.cpp)
    target_link_libraries(sync_smart_ptr_test PRIVATE sync_cpp Boost::ut)

    enable_testing()

    add_test(
        NAME sync_test
        COMMAND $<TARGET_FILE:sync_test>
    )

    add_test(
        NAME sync_container_test
        COMMAND $<TARGET_FILE:sync_container_test>
    )

    add_test(
        NAME sync_smart_ptr_test
        COMMAND $<TARGET_FILE:sync_smart_ptr_test>
    )
endif()
