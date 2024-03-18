cmake_minimum_required(VERSION 3.24)

project(sync_cpp_test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COLOR_DIAGNOSTICS ON)

find_package(ut CONFIG REQUIRED)

add_subdirectory(./lib/sync-cpp)

function(exe_test FILENAME)
    add_executable(${FILENAME} ${FILENAME}.cpp)
    target_link_libraries(${FILENAME} PRIVATE sync_cpp Boost::ut)

    add_test(
        NAME    ${FILENAME}
        COMMAND $<TARGET_FILE:${FILENAME}>
    )

    add_custom_command(
        TARGET     ${FILENAME} 
        POST_BUILD
        COMMAND    ${FILENAME}
    )
endfunction()

enable_testing()

exe_test(sync_test)
exe_test(sync_container_test)
exe_test(sync_smart_ptr_test)

