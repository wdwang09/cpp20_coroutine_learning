cmake_minimum_required(VERSION 3.16)
project(coroutine_learning)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(generator generator.cpp)
add_executable(optional optional.cpp)
add_executable(thread thread.cpp)
