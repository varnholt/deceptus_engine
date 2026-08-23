"""Breaks the update half of the frame into its phases on the desktop build.

Every previous profiling pass treated update as one number, because that is all the report showed.
On real switch hardware that number is about half the frame - roughly 10 ms of a 20.5 ms frame at
48 fps, against 2.8 ms in Ryujinx, which is why it looked negligible for so long. Ryujinx JITs the
guest's ARM code onto a desktop cpu, so it makes cpu work look cheap in exactly the same way it
makes fill look cheap.

    uv run --with pywin32 --with pillow python probe_update_sections.py [runs]

Two things differ from the render probes, both on purpose:

- **vsync stays ON.** The simulation runs a whole number of fixed 1/60 s steps per frame, so at
  60 fps a frame runs exactly one step and every phase reads as its true per step cost. With vsync
  off the desktop runs at several hundred fps, most frames consume no step at all, and every level
  phase comes out divided by however many frames went by between two steps. The update phases are
  measured before the swap, so vsync cannot inflate them the way it inflates a render section.
- **the render sections are worthless in this run** for the same reason - the vsync wait lands
  inside one of them. Use benchmark_render_profile.py for those.
"""

import json
import re
import shutil
import statistics
import subprocess
import sys
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "lab" / "map_render"))

import drive_desktop as desktop  # noqa: E402

CATACOMBS_LEVEL = "data/level-catacombs/level.json"
GAME_CONFIGURATION_PATH = desktop.SETTINGS_DIRECTORY / "game.json"
GAME_CONFIGURATION_BACKUP_PATH = desktop.SETTINGS_DIRECTORY / "game.json.update_probe_backup"
OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"

SETTLE_SECONDS = 6.0
SAMPLE_SECONDS = 30.0

VK_F10 = 0x79

UPDATE_SECTION_PATTERN = re.compile(r"profiling: update sections over (\d+) frames \|(.*)")
FPS_PATTERN = re.compile(r"profiling: fps ([0-9.]+) over \d+ frames \|(.*)")


def back_up_settings() -> None:
    if GAME_CONFIGURATION_PATH.exists() and not GAME_CONFIGURATION_BACKUP_PATH.exists():
        shutil.copy2(GAME_CONFIGURATION_PATH, GAME_CONFIGURATION_BACKUP_PATH)
        print(f"backed up {GAME_CONFIGURATION_PATH.name}")


def restore_settings() -> None:
    if GAME_CONFIGURATION_BACKUP_PATH.exists():
        shutil.copy2(GAME_CONFIGURATION_BACKUP_PATH, GAME_CONFIGURATION_PATH)
        GAME_CONFIGURATION_BACKUP_PATH.unlink()
        print("restored game.json")


def configure() -> None:
    configuration = json.loads(GAME_CONFIGURATION_PATH.read_text())
    section = configuration["GameConfiguration"]
    # see the module docstring: vsync on is what pins the frame to one simulation step
    section["vsync"] = True
    section["fullscreen"] = False
    GAME_CONFIGURATION_PATH.write_text(json.dumps(configuration, indent=4))


def install_save(checkpoint: int) -> None:
    if desktop.SAVE_STATE_PATH.exists() and not desktop.SAVE_STATE_BACKUP_PATH.exists():
        shutil.copy2(desktop.SAVE_STATE_PATH, desktop.SAVE_STATE_BACKUP_PATH)

    slots = json.loads(desktop.SAVE_STATE_PATH.read_text()) if desktop.SAVE_STATE_PATH.exists() else [{}, {}, {}]
    slots[0] = {
        "levelindex": 0,
        "checkpoints": {CATACOMBS_LEVEL: checkpoint},
        # null, not {} - Level::loadSaveState indexes a const nlohmann json
        "levelstate": None,
        "playerinfo": slots[0].get("playerinfo", {}) if slots else {},
    }
    slots[0]["playerinfo"]["name"] = "catacombs"
    desktop.SAVE_STATE_PATH.write_text(json.dumps(slots, indent=4))


def wait_for_level(stdout_path: Path, timeout_s: float) -> bool:
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        if stdout_path.exists():
            text = stdout_path.read_text(encoding="utf-8", errors="replace")
            if "level loading finished" in text:
                return True
        time.sleep(1.0)

    return False


def parse_entries(body: str) -> dict:
    entries = {}
    for entry in body.split("|"):
        entry = entry.strip()
        if not entry:
            continue
        name, _, value = entry.rpartition(" ")
        try:
            entries[name.strip()] = float(value)
        except ValueError:
            continue

    return entries


def parse_report(stdout_path: Path) -> dict:
    text = stdout_path.read_text(encoding="utf-8", errors="replace")

    update_reports = []
    for match in UPDATE_SECTION_PATTERN.finditer(text):
        entries = parse_entries(match.group(2))
        if entries:
            entries["_frames"] = float(match.group(1))
            update_reports.append(entries)

    frame_reports = []
    for match in FPS_PATTERN.finditer(text):
        entries = parse_entries(match.group(2))
        entries["fps"] = float(match.group(1))
        frame_reports.append(entries)

    return {"update": update_reports, "frames": frame_reports}


def run_once(run_index: int, checkpoint: int) -> dict:
    configure()
    install_save(checkpoint)

    executable = REPO_ROOT / "build_rel" / "deceptus.exe"
    stdout_path = OUTPUT_DIRECTORY / f"stdout_update_cp{checkpoint}_{run_index}.txt"

    with stdout_path.open("w", encoding="utf-8", errors="replace") as log_file:
        process = subprocess.Popen([str(executable)], cwd=str(REPO_ROOT), stdout=log_file, stderr=subprocess.STDOUT)
        try:
            print("  waiting for the main menu")
            time.sleep(14)

            handle = None
            for attempt in range(6):
                handle = desktop.focus_window()
                if not handle:
                    print("  no game window - did it die during start up?")
                    return {"loaded": False}

                desktop.send_key(handle, desktop.VK_RETURN, settle_s=2.0)
                desktop.send_key(handle, desktop.VK_RETURN, settle_s=2.0)

                print(f"  waiting for the level to load (attempt {attempt + 1})")
                if wait_for_level(stdout_path, timeout_s=25.0):
                    break

                print("  still in the menu, retrying")
            else:
                print("  FAILED to load the level - not sampling")
                return {"loaded": False}

            # F10 opens the profiling window; without it the desktop build writes no report at all
            desktop.send_key(handle, VK_F10, settle_s=1.0)

            time.sleep(SETTLE_SECONDS)
            print(f"  sampling for {SAMPLE_SECONDS:.0f}s")
            time.sleep(SAMPLE_SECONDS)
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=10)
            subprocess.run(["taskkill", "/F", "/IM", "deceptus.exe"], capture_output=True)

    report = parse_report(stdout_path)
    report["loaded"] = True
    report["log"] = stdout_path.name
    return report


def median_of(reports: list[dict], group: str, key: str) -> float | None:
    values = [entry[key] for report in reports for entry in report.get(group, []) if key in entry]
    return statistics.median(values) if values else None


def main() -> int:
    run_count = int(sys.argv[1]) if len(sys.argv) > 1 and sys.argv[1].isdigit() else 1
    checkpoint = int(sys.argv[2]) if len(sys.argv) > 2 and sys.argv[2].isdigit() else 3

    executable = REPO_ROOT / "build_rel" / "deceptus.exe"
    if not executable.exists():
        print(f"desktop build not found at {executable}")
        return 1

    OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    back_up_settings()

    reports = []
    try:
        for run_index in range(run_count):
            print(f"run {run_index + 1}/{run_count} at checkpoint {checkpoint}")
            report = run_once(run_index, checkpoint)
            if report.get("loaded"):
                reports.append(report)
    finally:
        restore_settings()
        desktop.restore_save_state()

    if not reports:
        print("no usable runs")
        return 1

    # skip nothing here: the update phases are not distorted by what happened before F10 the way the
    # per layer fill counters are, because they are averaged over the report window rather than
    # accumulated since the counter was last cleared
    names = []
    for report in reports:
        for entry in report.get("update", []):
            for name in entry:
                if name not in names and not name.startswith("_"):
                    names.append(name)

    print()
    print(f"fps {median_of(reports, 'frames', 'fps')}  update {median_of(reports, 'frames', 'update')} ms")
    print()
    print("update phases, worst first (ms per frame, median over reports)")
    ranked = sorted(
        ((name, median_of(reports, "update", name)) for name in names),
        key=lambda item: -(item[1] or 0.0),
    )
    for name, value in ranked:
        marker = "  <- span, contains the level phases" if name == "level update" else ""
        print(f"  {value:7.3f}  {name}{marker}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
