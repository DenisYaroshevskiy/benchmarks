cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 14)

project(root)

include_directories(./)

add_subdirectory(tests)
add_subdirectory(third_party)
add_subdirectory(benchmarks)
