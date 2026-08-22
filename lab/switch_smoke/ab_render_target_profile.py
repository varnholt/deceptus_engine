"""A/Bs the render target profile in Ryujinx, one NRO and two guest configs.

The profile lives in the guest's game.json, so "full" and "reduced" are two runs of the same build
rather than two builds - which removes the usual doubt about whether the two halves really were
different binaries.

Runs go in both orders (reduced, full, full, reduced). Interleaving alone is not enough: if every
pair runs the same way round and the machine degrades during the session, whichever half runs second
is systematically penalised, and that has flipped the sign of a result here before.

    uv run --with pywin32 python ab_render_target_profile.py [--sample-seconds 45]

Divide the emulator's frame rate by about 3 for a rough hardware figure. What the emulator cannot
see at all is fill: it re-issues the Tegra command buffers as Vulkan on a desktop gpu, so fragments
are far cheaper there than on the console - and fill is exactly what a smaller render target saves.
So read a flat result here as "no regression", not as "no gain".
"""

import argparse
import re
import statistics
import subprocess
import sys
from pathlib import Path

SCRIPT_DIRECTORY = Path(__file__).resolve().parent
MEAN_PATTERN = re.compile(r"in-level mean over (\d+) reports: fps ([\d.]+) \| update ([\d.]+) ms \| draw ([\d.]+) ms")
TARGET_PATTERN = re.compile(r"render target profile '(\w+)': (.*?)(?:\x1b|$)")


def run_once(profile_name: str, sample_seconds: float, monitor: int | None) -> dict | None:
    command = [
        sys.executable,
        str(SCRIPT_DIRECTORY / "profile_ryujinx.py"),
        "--render-target-profile",
        profile_name,
        "--sample-seconds",
        str(sample_seconds),
    ]
    if monitor is not None:
        command += ["--monitor", str(monitor)]

    print(f"\n=== {profile_name} ===", flush=True)
    completed = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace")
    print(completed.stdout[-2000:])
    if completed.returncode != 0:
        print(f"  run FAILED with {completed.returncode}")
        return None

    mean_match = MEAN_PATTERN.search(completed.stdout)
    if not mean_match:
        print("  no in-level mean in the output")
        return None

    target_match = TARGET_PATTERN.search(completed.stdout)
    return {
        "profile": profile_name,
        "reports": int(mean_match.group(1)),
        "fps": float(mean_match.group(2)),
        "update_ms": float(mean_match.group(3)),
        "draw_ms": float(mean_match.group(4)),
        "targets": target_match.group(2).strip() if target_match else "",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sample-seconds", type=float, default=45.0)
    parser.add_argument("--monitor", type=int, default=None)
    parser.add_argument("--rounds", type=int, default=1, help="how many times to run the both-orders pair")
    args = parser.parse_args()

    order = ["reduced", "full", "full", "reduced"] * args.rounds

    results = [run_once(profile_name, args.sample_seconds, args.monitor) for profile_name in order]
    results = [result for result in results if result]

    print("\n--- per run ---")
    for result in results:
        print(f"  {result['profile']:8s} fps {result['fps']:7.2f} | draw {result['draw_ms']:6.2f} ms | update {result['update_ms']:5.2f} ms")

    print("\n--- pooled ---")
    baseline_draw = None
    for profile_name in ("full", "reduced"):
        group = [result for result in results if result["profile"] == profile_name]
        if not group:
            print(f"  {profile_name}: no result")
            continue

        median_fps = statistics.median(result["fps"] for result in group)
        median_draw = statistics.median(result["draw_ms"] for result in group)
        if baseline_draw is None:
            baseline_draw = median_draw

        print(
            f"  {profile_name:8s} n={len(group)} fps {median_fps:7.2f} (hardware guess {median_fps / 3.0:5.2f})"
            f" | draw {median_draw:6.2f} ms ({baseline_draw / median_draw:4.2f}x)"
            f" | targets {group[0]['targets']}"
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
