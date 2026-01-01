add_executable(main main.cpp)
target_link_libraries(main PRIVATE sync-cpp)

add_executable(main_broken main.cpp)
target_link_libraries(main_broken PRIVATE sync-cpp)
target_compile_definitions(main_broken PRIVATE SYNC_CPP_EXAMPLE_USE_MOCK=1)

# thread sanitizer
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(main PRIVATE /fsanitize=thread)
    target_link_options(main PRIVATE /fsanitize=thread)

    target_compile_options(main_broken PRIVATE /fsanitize=thread)
    target_link_options(main_broken PRIVATE /fsanitize=thread)
else()
    target_compile_options(main PRIVATE -fsanitize=thread)
    target_link_options(main PRIVATE -fsanitize=thread)

    target_compile_options(main_broken PRIVATE -fsanitize=thread)
    target_link_options(main_broken PRIVATE -fsanitize=thread)
endif()
