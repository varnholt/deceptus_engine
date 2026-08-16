cmake_minimum_required(VERSION 3.16)

# Smoke test for the Nintendo Switch homebrew target.
#
# Build via the repo root:
#   build_switch.bat lab/switch_smoke build_switch
#
# Produces switch_smoke.nro. Run it with `nxlink -s switch_smoke.nro` to see the
# reported GL driver capabilities on the host.

project(
    switch_smoke
    VERSION 1.0
    LANGUAGES CXX)

if(NOT NINTENDO_SWITCH)
    message(FATAL_ERROR "this project only builds for the Switch; pass -DCMAKE_TOOLCHAIN_FILE=$ENV{DEVKITPRO}/cmake/Switch.cmake")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(switch_smoke source/main.cpp)

target_include_directories(switch_smoke PRIVATE ${DEVKITPRO}/portlibs/switch/include)

# link order matters for these static libs: glad resolves through EGL, which resolves
# through mesa's dispatch layer, which sits on the nouveau kernel interface
target_link_libraries(
    switch_smoke
    PRIVATE glad
            EGL
            glapi
            drm_nouveau)

nx_generate_nacp(
    switch_smoke.nacp
    NAME
    "Deceptus Switch Smoke Test"
    AUTHOR
    "Deceptus"
    VERSION
    "1.0.0")

nx_create_nro(switch_smoke NACP ${CMAKE_CURRENT_BINARY_DIR}/switch_smoke.nacp)
