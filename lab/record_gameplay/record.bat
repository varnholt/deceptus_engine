@echo off
cd /d "%~dp0"
uv run --project . pytest . -s
