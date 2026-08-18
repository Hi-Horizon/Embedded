cmake_minimum_required(VERSION 3.22)

set(CMAKE_PROJECT_NAME commons)
project(${CMAKE_PROJECT_NAME})

add_library(commons STATIC)

target_sources(commons PRIVATE
    buffer/buffer.c
    CANparser/CANparser.c
)

target_include_directories(commons PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

idf_component_register(
        SRCS
        INCLUDE_DIRS "."
)

