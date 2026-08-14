@echo off
docker run --rm -v "%CD%:/workspace" -w /workspace devkitpro/devkita64 bash -c "bash docker/build_switch.sh %1 %2"
