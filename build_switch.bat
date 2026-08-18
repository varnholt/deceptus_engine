@echo off
rem Builds a project for the Nintendo Switch homebrew target inside the devkitPro container.
rem
rem   build_switch.bat                                  -> the game into build_switch_engine
rem   build_switch.bat lab/switch_smoke  build_switch   -> a diagnostic project instead
rem
rem Building the game with no arguments matches build_wasm.bat, and is the case that actually
rem gets typed. docker/build_switch.sh still defaults to the smoke test when called directly,
rem because that is the toolchain canary CI builds first; CI passes both arguments explicitly
rem either way.
rem
rem The image tag is pinned rather than left at latest: it carries the compiler, libnx and the
rem portlibs, so a moving tag would change what a clean build produces. .github/workflows/switch.yml
rem names the same tag, so local builds and CI agree.

set SOURCE_DIRECTORY=%1
set BUILD_DIRECTORY=%2
if "%SOURCE_DIRECTORY%" == "" set SOURCE_DIRECTORY=.
if "%BUILD_DIRECTORY%" == "" set BUILD_DIRECTORY=build_switch_engine

docker run --rm -v "%CD%:/workspace" -w /workspace devkitpro/devkita64:20260219 bash -c "bash docker/build_switch.sh %SOURCE_DIRECTORY% %BUILD_DIRECTORY%"
