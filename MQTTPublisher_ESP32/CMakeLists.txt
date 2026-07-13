cmake_minimum_required(VERSION 3.16.0)
list(APPEND EXTRA_COMPONENT_DIRS ../Commons)
list(APPEND EXTRA_COMPONENT_DIRS components/esp-crc-suite)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(MQTTPublisher_ESP32)
