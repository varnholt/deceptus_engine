@echo off
docker run --rm -v "%CD%:/workspace" -w /workspace emscripten/emsdk bash -c "rm -rf build_wasm && mkdir -p build_wasm && cd build_wasm && emcmake cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)"
