add_executable(test_loopback test_loopback.cpp)
target_link_libraries(test_loopback PRIVATE canbus)

add_test(NAME loopback COMMAND test_loopback)
set_tests_properties(loopback PROPERTIES TIMEOUT 10)
