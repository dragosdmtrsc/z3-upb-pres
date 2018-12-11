cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 11)
add_executable(timetable main.cpp entities.cpp entities.h)
target_link_libraries (timetable ${Z3_LIB})
