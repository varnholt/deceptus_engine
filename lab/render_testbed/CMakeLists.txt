cmake_minimum_required(VERSION 3.20)
project(RenderTestbedProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# sFML configuration
set(SFML_BUILD_AUDIO ON CACHE BOOL "" FORCE)
set(SFML_BUILD_GRAPHICS ON CACHE BOOL "" FORCE)
set(SFML_BUILD_WINDOW ON CACHE BOOL "" FORCE)
set(SFML_BUILD_SYSTEM ON CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK ON CACHE BOOL "" FORCE)
set(SFML_USE_STATIC_STD_LIBS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

# fetch SFML 3
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.x
)

FetchContent_MakeAvailable(SFML)

set(IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/../../thirdparty/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/../../thirdparty/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/../../thirdparty/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/../../thirdparty/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/../../thirdparty/imgui/imgui-SFML.cpp
)

add_executable(RenderTestbed
    main.cpp
    testmechanism.cpp

    # ImGui sources
    ${IMGUI_SOURCES}

    # framework
    ../../src/framework/image/layer.h
    ../../src/framework/image/layer.cpp
    ../../src/framework/image/psd.h
    ../../src/framework/image/psd.cpp
    ../../src/framework/image/image.h
    ../../src/framework/image/image.cpp
    ../../src/framework/image/tga.h
    ../../src/framework/image/tga.cpp
    ../../src/framework/easings/easings.h
)

target_include_directories(RenderTestbed PRIVATE ${sfml_SOURCE_DIR}/include)

target_include_directories(RenderTestbed PRIVATE
    ${sfml_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../../thirdparty/imgui
)

target_link_libraries(RenderTestbed PRIVATE sfml-graphics sfml-window sfml-system)
