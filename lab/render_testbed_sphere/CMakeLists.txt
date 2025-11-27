cmake_minimum_required(VERSION 3.20)
project(RenderTestbedProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# SFML configuration
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

# Add all the copied files from the shmup project
set(VBO_SOURCES
    vbos/vbocube.cpp
    vbos/vbolinestrip.cpp
    vbos/vbomesh.cpp
    vbos/vboplane.cpp
    vbos/vboquadstrip.cpp
    vbos/vbosphere.cpp
    vbos/vbovoxelmesh.cpp
)

set(OPENGL_SOURCES
    opengl/glutils.cpp
    opengl/glslprogram.cpp
)

set(GAME_SOURCES
    game/camera.cpp
    game/shaderpool.cpp
)

set(INTERFACE_SOURCES
    interfaces/drawable.cpp
    interfaces/transformable.cpp
)

set(IMAGE_SOURCES
    image/tgaio.cpp
    image/bmpreader.cpp
    image/png.cpp
)

set(STRUCTURES_SOURCES
    structures/voxel.cpp
    structures/voxelgrid.cpp
)

add_executable(RenderTestbed
    main.cpp
    testmechanism.cpp

    # ImGui sources
    ${IMGUI_SOURCES}

    # VBO sources
    ${VBO_SOURCES}

    # OpenGL sources
    ${OPENGL_SOURCES}

    # Game sources
    ${GAME_SOURCES}

    # Interface sources
    ${INTERFACE_SOURCES}

    # Structures sources
    ${STRUCTURES_SOURCES}

    # Image sources
    ${IMAGE_SOURCES}
)

target_include_directories(RenderTestbed PRIVATE
    ${SFML_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../../thirdparty/imgui
    ${CMAKE_SOURCE_DIR}/glm
    ${CMAKE_SOURCE_DIR}/shmup/thirdparty/glm
    ${CMAKE_SOURCE_DIR}/shmup/thirdparty/glew/include  # Add GLEW include directory
    ${CMAKE_SOURCE_DIR}  # Add project root for relative includes like interfaces/drawable.h
    vbos
    opengl
    game
    structures
    image
)

# Find and link OpenGL
find_package(OpenGL REQUIRED)

# Add GLEW include directory (for the header files that VBOs use)
target_include_directories(RenderTestbed PRIVATE
    ${CMAKE_SOURCE_DIR}/shmup/thirdparty/glew/include
)

# For Windows, we can use the shared library (DLL) approach with import library
# The glew32.lib is actually the import library for the DLL
add_library(glew32 SHARED IMPORTED)
set_target_properties(glew32 PROPERTIES
    IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/shmup/thirdparty/glew/lib/Release/x64/glew32.lib"
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/shmup/thirdparty/glew/bin/Release/x64/glew32.dll"
)

target_link_libraries(RenderTestbed PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    OpenGL::GL
    glew32
)

# Copy the DLL to the output directory
add_custom_command(TARGET RenderTestbed POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/shmup/thirdparty/glew/bin/Release/x64/glew32.dll"
    $<TARGET_FILE_DIR:RenderTestbed>
)
