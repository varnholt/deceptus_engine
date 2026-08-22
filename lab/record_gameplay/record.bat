@echo off
cd /d "%~dp0"
uv run --project . pytest ../../tests/desktop/test_record_gameplay.py -s
