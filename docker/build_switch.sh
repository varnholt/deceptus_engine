#!/bin/bash
set -e

# Builds a CMake project for the Nintendo Switch homebrew target inside the
# devkitpro/devkita64 container. Defaults to the smoke test until the engine
# itself builds for Switch (the SDL3 Switch backend is a prerequisite).

SOURCE_DIRECTORY="${1:-lab/switch_smoke}"
BUILD_DIRECTORY="${2:-build_switch}"

echo "source directory: /workspace/${SOURCE_DIRECTORY}"
echo "build directory:  /workspace/${BUILD_DIRECTORY}"

cmake \
    -S "/workspace/${SOURCE_DIRECTORY}" \
    -B "/workspace/${BUILD_DIRECTORY}" \
    -DCMAKE_TOOLCHAIN_FILE="${DEVKITPRO}/cmake/Switch.cmake" \
    -DCMAKE_BUILD_TYPE=Release

cmake --build "/workspace/${BUILD_DIRECTORY}" -- -j"$(nproc)"

echo
echo "build artifacts:"
find "/workspace/${BUILD_DIRECTORY}" -name "*.nro" -o -name "*.elf" | sed 's|^|  |'
