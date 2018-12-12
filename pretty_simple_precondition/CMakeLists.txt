cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 11)
add_executable(pretty_simple_precondition main.cpp)
target_link_libraries (pretty_simple_precondition ${Z3_LIB})
