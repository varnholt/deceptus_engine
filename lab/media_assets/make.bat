@echo off
cd /d "%~dp0..\.."
uv run --project lab/media_assets lab/media_assets/make_media_assets.py %*
