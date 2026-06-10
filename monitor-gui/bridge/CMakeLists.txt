add_executable(can_monitor_bridge src/bridge.cpp)
target_link_libraries(can_monitor_bridge PRIVATE canbus)
