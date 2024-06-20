
cmake_minimum_required(VERSION 2.8)

project(LocalDGP)

aux_source_directory(./src DIR_SRCS)
SET(CMAKE_CXX_FLAGS "-std=c++11 -O3")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")


include_directories(/usr/local/include/igraph)
link_directories(/usr/local/lib)

FIND_PACKAGE(OpenMP REQUIRED)
if (OPENMP_FOUND)
    message("OPENMP FOUND")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif ()


add_executable(LocalDGP
        src/main.cpp
        src/partitioner.cpp
        src/disk_graph.cpp
        src/util.cpp
        src/memory_monitor.cpp
        src/memory_monitor.h)
add_executable(stream
        src/memory_monitor.cpp
        src/memory_monitor.h
        src/streaming/stream_graph.cpp
        src/streaming/stream_graph.hpp
        src/streaming/partitioner_stream.cpp
        src/streaming/partitioner_stream.hpp
        src/streaming/main_stream.cpp
        src/util.cpp
        src/memory_monitor.cpp
        src/memory_monitor.h)
add_executable(g_convert
        src/g_convert.cpp
        src/util.cpp)
add_executable(graph_gen
        src/graph_gen/graph_gen.cpp
        src/util.cpp)
add_executable(METIS_cal
        src/METIS_calculate/main_mc.cpp
        src/METIS_calculate/mgraph.cpp
        src/METIS_calculate/mgraph.h
        src/util.cpp
        )


target_link_libraries(LocalDGP igraph glog gflags)
target_link_libraries(stream igraph glog gflags)
target_link_libraries(g_convert igraph glog gflags)
target_link_libraries(graph_gen igraph glog gflags)
target_link_libraries(METIS_cal igraph glog gflags)
