cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/components)

if(IDF_TARGET STREQUAL "esp32s3" or IDF_TARGET STREQUAL "esp32s2")
    list(APPEND EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/extra_components)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(blink)
