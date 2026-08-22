"""Prices a render target profile on the desktop build, from the counters rather than from fps.

The level render targets fall into groups - image, lighting, normal, atmosphere - and
RenderTargetProfile sizes each group relative to the window image. "full" renders every group at
the window resolution, "reduced" halves lighting, normals and the atmosphere.

fps on this machine is worth nothing for this: the desktop is not fill bound (dropping every target
to a quarter moved it by 0.99x), and runs drift by more than any effect we are chasing. What does
transfer to the console is the fill counter, which now scales each pass by the size of the target it
rasterises into - so a pass moved to a half size target reads as a quarter of the fill.

    uv run --with pywin32 --with pillow python benchmark_render_profile.py [full reduced ...]

Runs are interleaved in both orders, because a session that degrades penalises whichever half runs
second - that alone once flipped the sign of a 3% result.
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
GAME_CONFIGURATION_BACKUP_PATH = desktop.SETTINGS_DIRECTORY / "game.json.profile_backup"
OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"

SETTLE_SECONDS = 6.0
SAMPLE_SECONDS = 30.0

VK_F10 = 0x79

OVERDRAW_PATTERN = re.compile(
    r"overdraw tiles ([0-9.]+)x \(normal pass ([0-9.]+)x\), ao ([0-9.]+)x, image layers ([0-9.]+)x, total ([0-9.]+)x"
)
SECTION_LINE_PATTERN = re.compile(r"profiling: sections over \d+ frames \|(.*)")
TARGET_SIZE_PATTERN = re.compile(r"render target profile '(\w+)': (.*)")


def back_up_settings() -> None:
    if GAME_CONFIGURATION_PATH.exists() and not GAME_CONFIGURATION_BACKUP_PATH.exists():
        shutil.copy2(GAME_CONFIGURATION_PATH, GAME_CONFIGURATION_BACKUP_PATH)
        print(f"backed up {GAME_CONFIGURATION_PATH.name}")


def restore_settings() -> None:
    if GAME_CONFIGURATION_BACKUP_PATH.exists():
        shutil.copy2(GAME_CONFIGURATION_BACKUP_PATH, GAME_CONFIGURATION_PATH)
        GAME_CONFIGURATION_BACKUP_PATH.unlink()
        print("restored game.json")


def configure(profile_name: str) -> None:
    configuration = json.loads(GAME_CONFIGURATION_PATH.read_text())
    section = configuration["GameConfiguration"]
    section["vsync"] = False
    section["fullscreen"] = False
    section["render_target_profile"] = profile_name
    GAME_CONFIGURATION_PATH.write_text(json.dumps(configuration, indent=4))


def install_save() -> None:
    if desktop.SAVE_STATE_PATH.exists() and not desktop.SAVE_STATE_BACKUP_PATH.exists():
        shutil.copy2(desktop.SAVE_STATE_PATH, desktop.SAVE_STATE_BACKUP_PATH)

    slots = json.loads(desktop.SAVE_STATE_PATH.read_text()) if desktop.SAVE_STATE_PATH.exists() else [{}, {}, {}]
    slots[0] = {
        "levelindex": 0,
        "checkpoints": {CATACOMBS_LEVEL: 3},
        # null, not {} - Level::loadSaveState indexes a const nlohmann json
        "levelstate": None,
        "playerinfo": slots[0].get("playerinfo", {}) if slots else {},
    }
    slots[0]["playerinfo"]["name"] = "catacombs"
    desktop.SAVE_STATE_PATH.write_text(json.dumps(slots, indent=4))


def wait_for_level(stdout_path: Path, timeout_s: float) -> bool:
    """Waits until the log says the level finished loading, rather than assuming it did."""
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        if stdout_path.exists():
            text = stdout_path.read_text(encoding="utf-8", errors="replace")
            if "level loading finished" in text:
                return True
        time.sleep(1.0)

    return False


def parse_report(stdout_path: Path) -> dict:
    text = stdout_path.read_text(encoding="utf-8", errors="replace")

    target_sizes = ""
    size_match = TARGET_SIZE_PATTERN.search(text)
    if size_match:
        target_sizes = size_match.group(2).strip()

    overdraws = []
    for match in OVERDRAW_PATTERN.finditer(text):
        overdraws.append(
            {
                "tiles": float(match.group(1)),
                "normal_pass": float(match.group(2)),
                "ao": float(match.group(3)),
                "image_layers": float(match.group(4)),
                "total": float(match.group(5)),
            }
        )

    sections = []
    for match in SECTION_LINE_PATTERN.finditer(text):
        entries = {}
        for entry in match.group(1).split("|"):
            entry = entry.strip()
            if not entry:
                continue
            name, _, value = entry.rpartition(" ")
            try:
                entries[name.strip()] = float(value)
            except ValueError:
                continue
        if entries:
            sections.append(entries)

    return {"target_sizes": target_sizes, "overdraws": overdraws, "sections": sections}


def run_once(profile_name: str) -> dict:
    configure(profile_name)
    install_save()

    executable = REPO_ROOT / "build_rel" / "deceptus.exe"
    run_index = len(list(OUTPUT_DIRECTORY.glob(f"stdout_profile_{profile_name}_*.txt")))
    stdout_path = OUTPUT_DIRECTORY / f"stdout_profile_{profile_name}_{run_index}.txt"

    with stdout_path.open("w", encoding="utf-8", errors="replace") as log_file:
        process = subprocess.Popen([str(executable)], cwd=str(REPO_ROOT), stdout=log_file, stderr=subprocess.STDOUT)
        try:
            print("  waiting for the main menu")
            time.sleep(14)

            # a run that never gets past the menu still produces a perfectly plausible report, so the
            # confirm presses are retried and the load is verified from the log before sampling
            handle = None
            for attempt in range(6):
                handle = desktop.focus_window()
                if not handle:
                    print("  no game window - did it die during start up?")
                    return {"profile": profile_name, "loaded": False}

                desktop.send_key(handle, desktop.VK_RETURN, settle_s=2.0)
                desktop.send_key(handle, desktop.VK_RETURN, settle_s=2.0)

                print(f"  waiting for the level to load (attempt {attempt + 1})")
                if wait_for_level(stdout_path, timeout_s=25.0):
                    break

                print("  still in the menu, retrying")
            else:
                print("  FAILED to load the level - not sampling")
                return {"profile": profile_name, "loaded": False}

            # F10 opens the profiling window, which is the only thing that makes the desktop build
            # write its per section costs and its fill counters to the log
            desktop.send_key(handle, VK_F10, settle_s=1.0)

            # the first seconds after a load still carry streaming and shader compilation
            time.sleep(SETTLE_SECONDS)

            print(f"  sampling for {SAMPLE_SECONDS:.0f}s")
            time.sleep(SAMPLE_SECONDS)

            # the capture carries the run index too: comparing two profiles needs a same profile pair
            # to establish the noise floor first, and one file per profile cannot hold that
            desktop.grab_window(handle, OUTPUT_DIRECTORY / f"profile_{profile_name}_{run_index}.png")
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=10)
            subprocess.run(["taskkill", "/F", "/IM", "deceptus.exe"], capture_output=True)

    report = parse_report(stdout_path)
    report["profile"] = profile_name
    report["loaded"] = True
    report["log"] = stdout_path.name
    return report


def median_of(reports: list[dict], key: str) -> float | None:
    values = [overdraw[key] for report in reports for overdraw in report.get("overdraws", [])]
    return statistics.median(values) if values else None


def median_section(reports: list[dict], name: str) -> float | None:
    values = [sections[name] for report in reports for sections in report.get("sections", []) if name in sections]
    return statistics.median(values) if values else None


def main() -> int:
    profiles = [argument for argument in sys.argv[1:] if not argument.startswith("-")] or ["full", "reduced"]

    executable = REPO_ROOT / "build_rel" / "deceptus.exe"
    if not executable.exists():
        print(f"desktop build not found at {executable}")
        return 1

    OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    back_up_settings()

    # both orders, so a machine that degrades during the session cannot be mistaken for a result
    order = profiles + list(reversed(profiles))

    results: list[dict] = []
    try:
        for profile_name in order:
            print(f"\n=== render target profile {profile_name} ===")
            results.append(run_once(profile_name))
    finally:
        restore_settings()
        desktop.restore_save_state()

    grouped: dict[str, list[dict]] = {}
    for result in results:
        if result.get("loaded"):
            grouped.setdefault(result["profile"], []).append(result)

    print("\n--- per profile ---")
    for profile_name in profiles:
        reports = grouped.get(profile_name, [])
        if not reports:
            print(f"  {profile_name}: NO RESULT - the level never loaded")
            continue

        print(f"  {profile_name}: targets {reports[0]['target_sizes']}")
        for key in ("tiles", "normal_pass", "ao", "image_layers", "total"):
            value = median_of(reports, key)
            print(f"    overdraw {key:13s} {value:6.2f}x" if value is not None else f"    overdraw {key:13s} n/a")

        for name in ("clear level targets", "background layers", "atmosphere resolve", "foreground layers", "lighting", "TOTAL"):
            value = median_section(reports, name)
            if value is not None:
                print(f"    section {name:20s} {value:6.3f} ms")

    return 0


if __name__ == "__main__":
    sys.exit(main())
