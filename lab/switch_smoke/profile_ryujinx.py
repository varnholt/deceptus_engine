"""Boots the Switch build in Ryujinx, walks it into the saved level and collects the profiling log.

The reports this harvests come from ProfilingUi's log flavour, which the Switch build switches on
at start up because the console has neither an F10 key to toggle it with nor a window to show it
in (see src/game/debug/profilingui.cpp).

A warning that cannot be repeated often enough: the numbers this produces are NOT a performance
proxy for real hardware. Ryujinx JITs guest AArch64 onto a desktop cpu and re-issues the Tegra
command buffers as Vulkan on a desktop gpu, so it is faster than a Switch by a wide and uneven
margin. What it is good for is checking that the instrument works in a real level, and for A/B
comparisons where both halves run on this same emulator.

Which is exactly why the harness insists on proving it reached the level. Anything that takes the
foreground away at the wrong moment -- a click, another window opening -- swallows the confirm
presses and leaves the game sitting in the main menu, where it happily produces profiling reports
that look fine and mean nothing. A menu frame updates in well under 0.1 ms because no physics runs;
a level frame is an order of magnitude above that, so IN_LEVEL_UPDATE_MS_MINIMUM tells the two
apart and the run fails loudly rather than handing back a menu measurement.
"""

import argparse
import json
import os
import re
import shutil
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from ryujinx_driver import (  # noqa: E402
    VK_D,
    VK_X,
    NRO_PATH,
    RyujinxSession,
    find_window,
    hold_key,
    move_window_to_monitor,
    send_key,
)

RYUJINX_SD_DIR = Path(os.environ["APPDATA"]) / "Ryujinx" / "sdcard" / "switch" / "deceptus"
RYUJINX_SD_LOG_DIR = RYUJINX_SD_DIR / "logs"
GUEST_CONFIG_PATH = RYUJINX_SD_DIR / "settings" / "game.json"
GUEST_CONFIG_BACKUP_PATH = GUEST_CONFIG_PATH.with_suffix(".json.profiling_backup")

FRAME_REPORT = re.compile(
    r"profiling: fps (?P<fps>[\d.]+) over (?P<frames>\d+) frames"
    r".*?update min [\d.]+ avg (?P<update_avg>[\d.]+)"
    r".*?draw min [\d.]+ avg (?P<draw_avg>[\d.]+)"
)
# a menu frame runs no physics and updates in under 0.1 ms, a level frame sits far above this
IN_LEVEL_UPDATE_MS_MINIMUM = 0.5


def prepare_guest_config(render_target_profile: str | None = None) -> bool:
    """Turns vsync off in the guest's config for the duration of the run, and picks a target profile.

    vsync belongs on for playing, and the engine now honours it on the console. For profiling it has
    to come off: with it on, every frame that would have taken less than 16.7 ms reports 16.7 ms, so
    the numbers pin to 60 fps and any improvement above that line becomes invisible. Worse, a frame
    that misses the deadline waits for the next one, so the rate quantises to 60/30/20 and a change
    that removed real work looks like it did nothing at all.

    The render target profile goes in the same file, which is what makes an A/B of it one NRO and two
    runs rather than two builds.
    """
    if not GUEST_CONFIG_PATH.exists():
        print(f"no guest config at {GUEST_CONFIG_PATH}, leaving vsync alone")
        return False

    if not GUEST_CONFIG_BACKUP_PATH.exists():
        shutil.copy2(GUEST_CONFIG_PATH, GUEST_CONFIG_BACKUP_PATH)

    config = json.loads(GUEST_CONFIG_PATH.read_text(encoding="utf-8"))
    config["GameConfiguration"]["vsync"] = False
    if render_target_profile:
        config["GameConfiguration"]["render_target_profile"] = render_target_profile
    GUEST_CONFIG_PATH.write_text(json.dumps(config, indent=4), encoding="utf-8")
    print("disabled vsync in the guest config for this run")
    if render_target_profile:
        print(f"render target profile: {render_target_profile}")
    return True


def restore_guest_vsync() -> None:
    if GUEST_CONFIG_BACKUP_PATH.exists():
        shutil.move(str(GUEST_CONFIG_BACKUP_PATH), str(GUEST_CONFIG_PATH))
        print("restored the guest config")


def newest_log() -> Path | None:
    if not RYUJINX_SD_LOG_DIR.is_dir():
        return None
    logs = sorted(RYUJINX_SD_LOG_DIR.glob("*.log"), key=lambda path: path.stat().st_mtime)
    return logs[-1] if logs else None


def read_reports(log_path: Path) -> list[dict]:
    reports = []
    for line in log_path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = FRAME_REPORT.search(line)
        if match:
            reports.append(
                {
                    "line": line.rstrip(),
                    "fps": float(match.group("fps")),
                    "update_avg": float(match.group("update_avg")),
                    "draw_avg": float(match.group("draw_avg")),
                }
            )
    return reports


def in_level(reports: list[dict]) -> list[dict]:
    return [report for report in reports if report["update_avg"] >= IN_LEVEL_UPDATE_MS_MINIMUM]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--boot-seconds", type=float, default=45.0, help="time to wait for the guest to reach the main menu")
    parser.add_argument("--load-seconds", type=float, default=45.0, help="time to wait for the level to finish loading")
    parser.add_argument("--sample-seconds", type=float, default=60.0, help="how long to leave the game running in the level")
    parser.add_argument("--walk", action="store_true", help="hold right while sampling instead of standing still")
    parser.add_argument("--monitor", type=int, default=None, help="index of the monitor to park the emulator on")
    parser.add_argument(
        "--nro",
        default=None,
        help="nro to boot instead of build_switch_engine/deceptus.nro, so two builds can be compared",
    )
    parser.add_argument(
        "--render-target-profile",
        default=None,
        choices=["full", "reduced"],
        help="render target profile to write into the guest config before the run",
    )
    args = parser.parse_args()

    previous_log = newest_log()
    prepare_guest_config(args.render_target_profile)

    try:
        return run_session(args, previous_log)
    finally:
        restore_guest_vsync()


def run_session(args, previous_log: Path | None) -> int:
    with RyujinxSession(nro_path=Path(args.nro) if args.nro else NRO_PATH):
        print(f"waiting {args.boot_seconds:.0f}s for the guest to boot")
        time.sleep(args.boot_seconds)

        # park the emulator on the secondary screen so a run does not sit on top of the main one
        if args.monitor is not None:
            emulator_window = find_window()
            if emulator_window and move_window_to_monitor(emulator_window, args.monitor):
                print(f"moved the emulator to monitor {args.monitor}")
            else:
                print(f"could not move the emulator to monitor {args.monitor}")

        log_path = newest_log()
        if log_path is None or log_path == previous_log:
            print("the guest never wrote a log, so it did not get far enough to profile")
            return 1

        print("confirming main menu")
        send_key(VK_X, settle_s=2.0)
        print("confirming file select")
        send_key(VK_X, settle_s=2.0)

        print(f"waiting up to {args.load_seconds:.0f}s for the level to start reporting")
        deadline = time.time() + args.load_seconds
        while time.time() < deadline and not in_level(read_reports(log_path)):
            # a stolen foreground eats the confirm presses, so keep offering them while waiting
            send_key(VK_X, settle_s=2.0)

        if not in_level(read_reports(log_path)):
            print(
                f"never reached the level - every report stayed below {IN_LEVEL_UPDATE_MS_MINIMUM} ms of update, "
                "which is the main menu. something took the foreground away from the emulator"
            )
            return 1

        print(f"in the level, sampling for {args.sample_seconds:.0f}s{' while walking' if args.walk else ' while standing still'}")
        sampling_started = len(read_reports(log_path))
        if args.walk:
            hold_key(VK_D, True)
        try:
            time.sleep(args.sample_seconds)
        finally:
            if args.walk:
                hold_key(VK_D, False)

        # the log thread flushes on a 100 ms tick, so give the last report a moment to reach the file
        time.sleep(1.0)

    reports = read_reports(log_path)[sampling_started:]
    level_reports = in_level(reports)

    print(f"\n--- {log_path} ---")
    for line in log_path.read_text(encoding="utf-8", errors="replace").splitlines():
        if "render target profile" in line:
            print(line.rstrip())

    # the section line is where a change to the draw path shows up; the frame rate alone cannot say
    # which pass moved
    section_lines = [line for line in log_path.read_text(encoding="utf-8", errors="replace").splitlines() if "sections over" in line]
    if section_lines:
        print(section_lines[-1].rstrip())
    if not level_reports:
        print("no in-level reports were collected while sampling")
        return 1

    for report in level_reports:
        print(report["line"])

    average_fps = sum(report["fps"] for report in level_reports) / len(level_reports)
    average_draw = sum(report["draw_avg"] for report in level_reports) / len(level_reports)
    average_update = sum(report["update_avg"] for report in level_reports) / len(level_reports)
    print(
        f"\nin-level mean over {len(level_reports)} reports: "
        f"fps {average_fps:.2f} | update {average_update:.2f} ms | draw {average_draw:.2f} ms"
    )
    if len(level_reports) != len(reports):
        print(f"note: {len(reports) - len(level_reports)} of {len(reports)} reports were menu frames and were left out")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
