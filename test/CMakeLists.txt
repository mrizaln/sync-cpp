include(cmake/fetched-libs.cmake)

function(exe_test FILENAME)
  add_executable(${FILENAME} ${FILENAME}.cpp)
  target_link_libraries(${FILENAME} PRIVATE sync-cpp Boost::ut)
  add_test(NAME ${FILENAME} COMMAND $<TARGET_FILE:${FILENAME}>)
  add_custom_command(TARGET ${FILENAME} POST_BUILD COMMAND ${FILENAME})
endfunction()

enable_testing()

exe_test(sync_test)
exe_test(sync_container_test)
exe_test(sync_smart_ptr_test)
exe_test(sync_opt_test)
