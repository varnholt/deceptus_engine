cmake_minimum_required(VERSION 3.20)
project(PathMerge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(path_merge
    main.cpp
    pathmerger.cpp
    painterpath.cpp
    pathsegments.cpp
    wingededge.cpp
    pathclipper.cpp
)
