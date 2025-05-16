cmake_minimum_required(VERSION 3.10)
project(wms)

set(SOURCES
    src/main.c
    src/log.c
    src/eventloop.c
)

add_executable(wms ${SOURCES})
target_include_directories(wms PRIVATE include)

target_compile_options(wms PRIVATE -Wall -Wextra -Wpedantic)

foreach(source_file IN LISTS SOURCES)
    get_filename_component(fname ${source_file} NAME)
    set_source_files_properties(${source_file} PROPERTIES COMPILE_DEFINITIONS "__WMS_FILE_NAME__=\"${fname}\"")
endforeach()

