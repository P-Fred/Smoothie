cmake_minimum_required(VERSION 2.8)

set(PACKAGE_VERSION "1.0")
set(PACKAGE_LICENSE "GPL")
SET(PROJECT_DESCRIPTION_SUMMARY "Smoothie proposes data-mining algorithms to compute smoothed mutual information along other measures to assess quality of functional dependencies")
set(CMAKE_BUILD_TYPE Release)

project(smoothed-info)

# Boost flags
set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED OFF)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.54.0 REQUIRED COMPONENTS program_options system) 

find_package(Threads)

add_compile_options(-std=c++20 -Wall -pedantic -fdiagnostics-color)

add_subdirectory(src)
add_subdirectory(algorithms)

