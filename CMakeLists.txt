project(system_util)
cmake_minimum_required(VERSION 3.10)

add_compile_options(-O0 -Wall -g -ggdb -std=c++11)

include_directories(./inc/)

link_directories(./lib/debug/)
link_directories(./lib/release/)
link_directories(./output/debug/lib/)
link_directories(./output/release/lib/)

add_subdirectory(./src/)
