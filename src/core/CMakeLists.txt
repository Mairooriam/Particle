cmake_minimum_required(VERSION 3.10)
project(core_lib)
# Gather all source files from platform and its subdirectories
file(GLOB CORE_SOURCES
"${CMAKE_CURRENT_SOURCE_DIR}/platform/*.c"
"${CMAKE_CURRENT_SOURCE_DIR}/platform/windows/*.c"
"${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_library(core STATIC ${CORE_SOURCES})
target_compile_definitions(core PRIVATE LOG_USE_COLOR)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)