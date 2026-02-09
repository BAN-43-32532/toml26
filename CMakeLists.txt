cmake_minimum_required(VERSION 4.0)
project(toml26 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_executable(toml26_demo demo.cpp)
target_include_directories(toml26_demo PRIVATE "${PROJECT_SOURCE_DIR}/include")

enable_testing()
add_subdirectory(test)
