cmake_minimum_required(VERSION 3.16)
project(CityRouter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure CMAKE_BUILD_TYPE is not set to Release so assert() remains active
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug ensures assert() is active)" FORCE)
endif()

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Static library named city_graph built from CityGraph.cpp
add_library(city_graph STATIC
    CityGraph.cpp
    CityGraph.h
    Route.h
)

# Set include directory to CityRouter root so tests can use #include "CityGraph.h"
target_include_directories(city_graph PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Test executable named test_graph built from tests/test_graph.cpp
add_executable(test_graph
    tests/test_graph.cpp
)

# Link test_graph to city_graph
target_link_libraries(test_graph PRIVATE
    city_graph
)
