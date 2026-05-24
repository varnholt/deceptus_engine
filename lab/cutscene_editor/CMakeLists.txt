cmake_minimum_required(VERSION 3.15)
project(cutscene_editor)

include(FetchContent)

# GLFW
FetchContent_Declare(
   glfw
   GIT_REPOSITORY https://github.com/glfw/glfw.git
   GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ImGui (full source with GLFW + OpenGL3 backends)
FetchContent_Declare(
   imgui
   GIT_REPOSITORY https://github.com/ocornut/imgui.git
   GIT_TAG        v1.91.0
)
FetchContent_MakeAvailable(imgui)

# nlohmann/json
FetchContent_Declare(
   nlohmann_json
   GIT_REPOSITORY https://github.com/nlohmann/json.git
   GIT_TAG        v3.11.3
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(cutscene_editor
   main.cpp
   document.h
   document.cpp
   editor.h
   editor.cpp
   ${imgui_SOURCE_DIR}/imgui.cpp
   ${imgui_SOURCE_DIR}/imgui_draw.cpp
   ${imgui_SOURCE_DIR}/imgui_tables.cpp
   ${imgui_SOURCE_DIR}/imgui_widgets.cpp
   ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
   ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(cutscene_editor PRIVATE
   ${imgui_SOURCE_DIR}
   ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(cutscene_editor PRIVATE
   glfw
   opengl32
   nlohmann_json::nlohmann_json
)

set_property(TARGET cutscene_editor PROPERTY CXX_STANDARD 23)

set_target_properties(cutscene_editor PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)
