@echo off
rem Builds a shipping-mode (DECEPTUS_DEVELOPMENT_MODE=OFF) desktop release and collects the
rem executable, data.pak, and SDL3.dll into release\ as a self-contained, shareable folder.
rem
rem   build_release.bat
rem
rem Deliberately uses CMake's default Visual Studio generator rather than Ninja: MSBuild sets up
rem its own MSVC toolchain, so this runs from a plain command prompt with no Developer Command
rem Prompt / vcvars environment required. Ninja needs that environment pre-loaded to configure
rem correctly, which is easy to get wrong outside a VS shell.

set RELEASE_DIRECTORY=release\windows

cmake -S . -B build_release -DDECEPTUS_DEVELOPMENT_MODE=OFF || exit /b 1
cmake --build build_release --config Release --target deceptus || exit /b 1

if not exist "%RELEASE_DIRECTORY%" mkdir "%RELEASE_DIRECTORY%"
copy /Y build_release\Release\deceptus.exe "%RELEASE_DIRECTORY%\"
copy /Y build_release\data.pak "%RELEASE_DIRECTORY%\"
copy /Y build_release\Release\SDL3.dll "%RELEASE_DIRECTORY%\"

echo.
echo Release build ready in .\%RELEASE_DIRECTORY%\
