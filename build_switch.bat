@echo off
rem Builds a project for the Nintendo Switch homebrew target inside the devkitPro container.
rem
rem   build_switch.bat                          -> lab/switch_smoke  into build_switch
rem   build_switch.bat . build_switch_engine    -> the game          into build_switch_engine
rem
rem The image tag is pinned rather than left at latest: it carries the compiler, libnx and the
rem portlibs, so a moving tag would change what a clean build produces. .github/workflows/switch.yml
rem names the same tag, so local builds and CI agree.
docker run --rm -v "%CD%:/workspace" -w /workspace devkitpro/devkita64:20260219 bash -c "bash docker/build_switch.sh %1 %2"
