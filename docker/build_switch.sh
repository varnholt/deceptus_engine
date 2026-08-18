#!/bin/bash
set -e

# Builds a CMake project for the Nintendo Switch homebrew target inside the
# devkitpro/devkita64 container.
#
# Defaults to the smoke test rather than the engine: it is the toolchain canary and builds
# in seconds. Pass "." to build the game itself.
#
# Paths are resolved against the current directory rather than a hardcoded /workspace, so
# the same script serves both callers. build_switch.bat mounts the repository at /workspace
# and runs with that as the working directory; GitHub Actions runs the container on its own
# workspace path, which is somewhere under /__w.

SOURCE_DIRECTORY="${1:-lab/switch_smoke}"
BUILD_DIRECTORY="${2:-build_switch}"

echo "working directory: $(pwd)"
echo "source directory:  ${SOURCE_DIRECTORY}"
echo "build directory:   ${BUILD_DIRECTORY}"

cmake \
    -S "${SOURCE_DIRECTORY}" \
    -B "${BUILD_DIRECTORY}" \
    -DCMAKE_TOOLCHAIN_FILE="${DEVKITPRO}/cmake/Switch.cmake" \
    -DCMAKE_BUILD_TYPE=Release

cmake --build "${BUILD_DIRECTORY}" -- -j"$(nproc)"

echo
echo "build artifacts:"
find "${BUILD_DIRECTORY}" -name "*.nro" -o -name "*.elf" | sed 's|^|  |'
