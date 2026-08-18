#!/bin/bash
set -e

echo "mkdir -p /workspace/build_linux"
mkdir -p /workspace/build_linux

cd /workspace/build_linux

echo "cmake /workspace -G Ninja -DCMAKE_BUILD_TYPE=Release"
cmake /workspace -G Ninja -DCMAKE_BUILD_TYPE=Release

echo "ninja"
ninja
