cmake_minimum_required(VERSION 3.11)

project(tinytasks)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-Werror=return-type -Wall -Wextra -Werror -fconcepts ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "-O0 -ggdb ${CMAKE_CXX_FLAGS}")

set(SRC
    main.cpp
    tests.cpp
    task/Scheduler.cpp
    task/ContinuationHolder.cpp
)

add_executable(tests ${SRC})
target_link_libraries(tests pthread)
