cmake_minimum_required(VERSION 3.10)

project(SPSCQueue LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall -Wextra")

find_package(Threads REQUIRED)

include_directories(include)

add_executable(spsc_benchmark 
    src/SPSCQueueBenchmark.cpp
)

target_link_libraries(spsc_benchmark PRIVATE Threads::Threads)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "CXX Flags: ${CMAKE_CXX_FLAGS}")