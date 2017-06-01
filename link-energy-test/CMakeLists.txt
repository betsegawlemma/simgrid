cmake_minimum_required(VERSION 3.5)
project(s4u_energy-consumption)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_SOURCE_DIR  /home/betsegaw/simgrid) 

include_directories(${CMAKE_SOURCE_DIR}/include)

link_directories(${CMAKE_SOURCE_DIR}/lib /opt/ns3/lib)

add_executable(sg_energy_consumption sg_energy_consumption.cpp)
add_executable(ns3_energy_consumption ns3_energy_consumption.cpp)
target_link_libraries(ns3_energy_consumption simgrid)
target_link_libraries(sg_energy_consumption simgrid)
