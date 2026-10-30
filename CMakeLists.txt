
cmake_minimum_required(VERSION 3.16)

project(CityRouter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# Debug configuration
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# ==========================================
# MEMBER 2 + MEMBER 3: CORE GRAPH LIBRARY
# ==========================================

add_library(city_graph STATIC
    CityGraph.cpp
    CityGraph.h
    Route.h
    AlternativeRoutes.h
    AlternativeRoutes.cpp
)

target_include_directories(city_graph PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ==========================================
# MEMBER 2: GRAPH TESTS
# ==========================================

add_executable(test_graph
    tests/test_graph.cpp
)

target_link_libraries(test_graph PRIVATE
    city_graph
)

# ==========================================
# MEMBER 3: ALTERNATIVE ROUTE TESTS
# ==========================================

add_executable(test_alternative_routes
    test_alternative_routes.cpp
)

target_link_libraries(test_alternative_routes PRIVATE
    city_graph
)

# ==========================================
# MEMBER 1: QT GUI APPLICATION
# ==========================================

add_executable(CityRouter
    main.cpp
    gui/mainwindow.cpp
    gui/mainwindow.h
    gui/CityMapWidget.cpp
    gui/CityMapWidget.h
)

target_include_directories(CityRouter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
)

target_link_libraries(CityRouter PRIVATE
    Qt6::Widgets
    Qt6::Network
    city_graph
)
