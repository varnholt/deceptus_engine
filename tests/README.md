# tests

The pytest suites, one directory per thing they test:

| suite | what it checks | needs |
|---|---|---|
| `switch/test_switch_build.py` | the .nro is a well formed homebrew binary, the Switch SDL backends were really linked rather than replaced by dummies, and the romfs carries the assets | a Switch build in `build_switch_engine` (or `DECEPTUS_SWITCH_BUILD_DIR`) |
| `wasm/test_wasm_browser.py` | the web build reaches the menu in a real browser without a severe console message | a build in `build_wasm`, Chrome; starts its own COOP/COEP server |
| `desktop/test_record_gameplay.py` | the desktop build launches, reaches a level and can be captured; produces the README recording | a desktop build, ffmpeg, Windows |
| `tools/test_path_merge.py` | the standalone path_merge binary agrees with the Qt5 reference | the two binaries in `lab/path_merge_tests` |

Run one with the project here, from the repository root:

    uv run --project tests pytest tests/switch/test_switch_build.py -v

Everything else stays where it was. `lab/` keeps the drivers and harnesses these suites are built
on (`lab/map_render/drive_desktop.py`, `lab/switch_smoke/ryujinx_driver.py`), the fixtures they read
(`lab/path_merge_tests/binary_*`, `lab/record_gameplay/config.json`), the launchers
(`lab/wasm_browser_test/run.bat`, `lab/record_gameplay/record.bat`) and the experiments themselves.
The Switch build is driven by `docker/build_switch.sh` and `build_switch.bat`, which are untouched by
this layout: they build `lab/switch_smoke` (the C++ toolchain canary) and the engine, and the suite
here only reads what they produced.
