cmake_minimum_required(VERSION 3.6)
project(queue)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES
    flow.cc
    flow.h
    main.cc
    queue.h
    utility.cc
    utility.h)

add_executable(queue ${SOURCE_FILES})
target_link_libraries(queue atomic)
target_link_libraries(queue pthread)
