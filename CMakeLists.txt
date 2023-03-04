cmake_minimum_required(VERSION 3.16)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(coroutine_learning)

if (UNIX)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
endif ()

# Define a generator.
add_executable(generator generator.cpp)
# Define std::future<std::optional<T>> with std::coroutine_traits.
add_executable(optional optional.cpp)
# Another version of defining std::optional<T> with coroutine.
add_executable(maybe maybe.cpp)
# Switching coroutine into another thread.
add_executable(thread thread.cpp)
# Define async task with co_await.
add_executable(task task.cpp)
