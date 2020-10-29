project(system_util)

set(LIBRARY_OUTPUT_PATH /home/ruanjian/workspace/project_pre/system_util/output/debug/lib)

aux_source_directory(. DIR_LIB_SRCS)

add_library(system_util ${DIR_LIB_SRCS})

target_link_libraries(system_util -lpthread)

