cmake_minimum_required(VERSION 3.20)
project(Cache_Coherence_)

set(CMAKE_CXX_STANDARD 11)


add_library(Caches cache.cc)
add_executable(Cache_Coherence_ main.cc)
target_link_libraries(Cache_Coherence_ Caches)