@echo off
echo Serving at http://localhost:8080/deceptus.html
docker run --rm -v "%CD%/build_wasm:/srv" -p 8080:8080 emscripten/emsdk python3 -m http.server 8080 --directory /srv
