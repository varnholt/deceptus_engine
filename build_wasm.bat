@echo off
setlocal

set BUILD_DIR=build_wasm

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

emcmake cmake -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release .
if errorlevel 1 (
    echo CMake configure failed.
    exit /b 1
)

cmake --build "%BUILD_DIR%"
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo.
echo Build complete. Output in %BUILD_DIR%\
echo Open %BUILD_DIR%\deceptus.html in a browser to test.
