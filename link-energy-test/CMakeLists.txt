cmake_minimum_required(VERSION 3.5)
project(s4u_energy-consumption)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_SOURCE_DIR  /home/betsegaw/simgrid) 

include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${CMAKE_SOURCE_DIR}/lib)

set(SIMULATOR_SOURCES s4u_energy-consumption.cpp)
add_executable(s4u_energy-consumption ${SIMULATOR_SOURCES})
target_link_libraries(s4u_energy-consumption simgrid)
