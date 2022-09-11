cmake_minimum_required(VERSION 3.20)
project(ECE_506)
set(CMAKE_CXX_STANDARD 11)
include_directories(include)
option(OPENMP_HARNESS "WITH_OPENMP_ON" ON )
option(MPI_HARNESS "WITH_MPI_ON" OFF) #Turn to ON here if compiling with MPI
option(HYBRID_HARNESS "WITH_HYBRID_HARNESS" OFF) #Turn to ON here if compiling with Hybrid_harness
add_library(sortlib src/arrayQueue.cpp src/bfs.cpp src/bitmap.cpp src/edgelist.cpp src/graph.cpp src/sort.cpp src/timer.cpp src/vertex.cpp)
add_executable(ECE_506 src/main.cpp)
find_package(OpenMP REQUIRED)
find_package(MPI)
if(OpenMP_FOUND)
    message(STATUS "OPENMP FOUND" )
else()
    message(FATAL_ERROR "OpenMP not found change compiler")
endif()
if(MPI_FOUND)
    message(STATUS "MPI FOUND")
else()
    message(STATUS "OpenMP Found MPI missing")
endif()

if(OPENMP_HARNESS)
    message("USING OPENMP_HARNESS")
    add_definitions(-DOPENMP_HARNESS)
    target_link_libraries(ECE_506  OpenMP::OpenMP_CXX sortlib)
endif()


if(MPI_HARNESS)
    message("USING MPI_HARNESS")
    add_definitions(-DMPI_HARNESS)
    target_link_libraries(ECE_506   MPI::MPI_CXX sortlib)

endif()
if(HYBRID_HARNESS)
    message("USING BOTH OPENMP AND MPI")
    add_definitions(-DHYBRID_HARNESS)
    target_link_libraries(ECE_506   OpenMP::OpenMP_CXX MPI::MPI_CXX sortlib)
endif()
unset(OPENMP_HARNESS CACHE)
unset(MPI_HARNESS CACHE)
unset(HYBRID_HARNESS CACHE)





