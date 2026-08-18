@echo off
cd /d "%~dp0"
uv run pytest test_wasm_browser.py -v -s %*
