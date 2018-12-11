cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 11)
add_executable(routing_table main.cpp)
target_link_libraries (routing_table ${Z3_LIB})
