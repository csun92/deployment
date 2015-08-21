cmake_minimum_required(VERSION 2.8)
project("MediPy")
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions(-D_OSX)
endif()

if(CMAKE_VERSION VERSION_LESS "2.8.3")
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
endif()

include("cmake/medipy.cmake")

add_subdirectory("lib")
add_subdirectory("plugins")
