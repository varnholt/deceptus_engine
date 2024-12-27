cmake_minimum_required(VERSION 3.15)

project(AnimationViewer)

# Set the directories for dependencies and source files
set(SRC_DIR "../../src")
set(LAB_DIR ".")
set(IMGUI_DIR "../../thirdparty/imgui")
set(SFML_DIR "${CMAKE_SOURCE_DIR}/../../thirdparty/sfml")
set(THIRDPARTY_DIR "../../thirdparty")


# Add the executable
add_executable(animation_viewer
    ${LAB_DIR}/main.cpp

    ${SRC_DIR}/game/animation/animation.cpp
    ${SRC_DIR}/game/animation/animationsettings.cpp
    ${SRC_DIR}/game/animation/animationpool.cpp
    ${SRC_DIR}/game/io/texturepool.cpp
    ${SRC_DIR}/framework/tools/log.cpp
)

# Include directories
include_directories(
    ${SRC_DIR}
    ${IMGUI_DIR}
    ${SFML_DIR}/include
    ${THIRDPARTY_DIR}
)

target_link_libraries(animation_viewer PRIVATE
     glu32
     opengl32
 )

# Link SFML libraries
set(SFML_LIB_DIR ${SFML_DIR}/lib)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_libraries(animation_viewer PRIVATE
        ${SFML_LIB_DIR}/sfml-audio.lib
        ${SFML_LIB_DIR}/sfml-graphics.lib
        ${SFML_LIB_DIR}/sfml-network.lib
        ${SFML_LIB_DIR}/sfml-system.lib
        ${SFML_LIB_DIR}/sfml-window.lib
    )
else()
    target_link_libraries(animation_viewer PRIVATE
        ${SFML_LIB_DIR}/sfml-audio-d.lib
        ${SFML_LIB_DIR}/sfml-graphics-d.lib
        ${SFML_LIB_DIR}/sfml-network-d.lib
        ${SFML_LIB_DIR}/sfml-system-d.lib
        ${SFML_LIB_DIR}/sfml-window-d.lib
    )
endif()

# Link ImGui files
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui-SFML.cpp
)


# Add ImGui sources to the target
target_sources(animation_viewer PRIVATE ${IMGUI_SOURCES})

# Link libraries to the target
target_link_libraries(animation_viewer PRIVATE ${SFML_LIBS})

# Set the executable's output directory
set_target_properties(animation_viewer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

# Enable C++23
set_property(TARGET animation_viewer PROPERTY CXX_STANDARD 23)

# Add post-build commands to copy required SFML DLLs to the output directory
add_custom_command(TARGET animation_viewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${SFML_DIR}/bin/sfml-graphics-2.dll
    ${SFML_DIR}/bin/sfml-window-2.dll
    ${SFML_DIR}/bin/sfml-system-2.dll
    $<TARGET_FILE_DIR:animation_viewer>
)
