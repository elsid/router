cmake_minimum_required(VERSION 3.12)

project(router VERSION 0.1 LANGUAGES CXX)

option(ROUTER_BUILD_EXAMPLES "Enable examples build" OFF)

set(CMAKE_CXX_EXTENSIONS OFF)

if(ROUTER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
