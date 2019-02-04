cmake_minimum_required(VERSION 3.12)
project(simple_socket_client)

set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(simple_socket_client main.cpp)
