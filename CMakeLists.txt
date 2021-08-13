cmake_minimum_required (VERSION 3.10)

project (Heeb)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

include_directories(${PROJECT_SOURCE_DIR}/include)

aux_source_directory (./src DIR_HEEB_SRCS)

add_executable(WebServer ${DIR_HEEB_SRCS})

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
