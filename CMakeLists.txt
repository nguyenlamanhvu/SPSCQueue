cmake_minimum_required(VERSION 3.14)
project(SPSCQueue 
    VERSION 1.0.0 
    DESCRIPTION "High-performance lock-free SPSC queue"
    LANGUAGES CXX
)

add_library(spsc_queue INTERFACE)
add_library(SPSCQueue::spsc_queue ALIAS spsc_queue)

target_include_directories(spsc_queue INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(spsc_queue INTERFACE cxx_std_17)

option(SPSC_BUILD_BENCHMARK "Build the benchmark" ON)

if(SPSC_BUILD_BENCHMARK)
    add_executable(spsc_benchmark benchmarks/main.cpp)
    target_link_libraries(spsc_benchmark PRIVATE spsc_queue pthread)
    
    target_compile_options(spsc_benchmark PRIVATE -O3 -march=native)
    
    set_target_properties(spsc_benchmark PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
endif()

install(TARGETS spsc_queue EXPORT SPSCQueueTargets)
install(DIRECTORY include/SPSCQueue DESTINATION include)