@echo off
cd /d "%~dp0"
uv run pytest ../../tests/wasm/test_wasm_browser.py -v -s %*
