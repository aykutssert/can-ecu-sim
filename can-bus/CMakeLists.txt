add_library(canbus
    src/can_socket.cpp
    src/frame.cpp
)
target_include_directories(canbus PUBLIC include)

add_executable(can_send src/tools/can_send_tool.cpp)
target_link_libraries(can_send PRIVATE canbus)

add_executable(can_dump src/tools/can_dump_tool.cpp)
target_link_libraries(can_dump PRIVATE canbus)

add_subdirectory(tests)
