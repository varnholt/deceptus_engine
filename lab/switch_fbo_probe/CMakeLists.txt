cmake_minimum_required(VERSION 3.16)

# Framebuffer-object probe for the Nintendo Switch homebrew target.
#
# Build via the repo root:
#   build_switch.bat lab/switch_fbo_probe build_switch_fbo_probe
#
# Produces switch_fbo_probe.nro. Run it in Ryujinx via lab/switch_smoke/run_ryujinx.ps1;
# the results appear in the guest log as "[fbo-probe]" lines.

project(
    switch_fbo_probe
    VERSION 1.0
    LANGUAGES CXX)

if(NOT NINTENDO_SWITCH)
    message(FATAL_ERROR "this project only builds for the Switch; pass -DCMAKE_TOOLCHAIN_FILE=$ENV{DEVKITPRO}/cmake/Switch.cmake")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(switch_fbo_probe source/main.cpp)

target_include_directories(switch_fbo_probe PRIVATE ${DEVKITPRO}/portlibs/switch/include)

# link order matters for these static libs: glad resolves through EGL, which resolves
# through mesa's dispatch layer, which sits on the nouveau kernel interface
target_link_libraries(
    switch_fbo_probe
    PRIVATE glad
            EGL
            glapi
            drm_nouveau)

nx_generate_nacp(
    switch_fbo_probe.nacp
    NAME
    "Deceptus Switch FBO Probe"
    AUTHOR
    "Deceptus"
    VERSION
    "1.0.0")

nx_create_nro(switch_fbo_probe NACP ${CMAKE_CURRENT_BINARY_DIR}/switch_fbo_probe.nacp)
