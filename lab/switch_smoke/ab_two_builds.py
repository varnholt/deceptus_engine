"""A/Bs two Switch builds in Ryujinx and reports the frame rate and the per section draw costs.

    uv run --with pywin32 --with pillow python ab_two_builds.py A.nro B.nro [--sample-seconds 45]

Both NROs are booted in turn, in both orders, because interleaving alone is not enough: if every
pair runs the same way round and the machine degrades during the session, whichever half runs second
is penalised, and that has flipped the sign of a result here before.

The emulator is not a hardware proxy - it re-issues the Tegra command buffers as Vulkan on a desktop
gpu, so fragments are far cheaper there than on the console while per draw and per target switch
translation is far more expensive. Dividing its frame rate by three is the rule of thumb. What it
measures honestly is the submission side of the draw path, and the section line says which pass moved.
"""

import argparse
import re
import statistics
import subprocess
import sys
from pathlib import Path

SCRIPT_DIRECTORY = Path(__file__).resolve().parent
MEAN_PATTERN = re.compile(r"in-level mean over (\d+) reports: fps ([\d.]+) \| update ([\d.]+) ms \| draw ([\d.]+) ms")
SECTION_PATTERN = re.compile(r"sections over \d+ frames \|(.*)")


def parse_sections(text: str) -> dict:
    match = SECTION_PATTERN.search(text)
    if not match:
        return {}

    sections = {}
    for entry in match.group(1).split("|"):
        entry = entry.strip()
        if not entry:
            continue
        name, _, value = entry.rpartition(" ")
        try:
            sections[name.strip()] = float(value)
        except ValueError:
            continue
    return sections


def run_once(label: str, nro_path: Path, sample_seconds: float, monitor: int | None) -> dict | None:
    command = [
        sys.executable,
        str(SCRIPT_DIRECTORY / "profile_ryujinx.py"),
        "--nro",
        str(nro_path),
        "--sample-seconds",
        str(sample_seconds),
    ]
    if monitor is not None:
        command += ["--monitor", str(monitor)]

    print(f"\n=== {label} ===", flush=True)
    completed = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if completed.returncode != 0:
        print(completed.stdout[-1500:])
        print(f"  run FAILED with {completed.returncode}")
        return None

    mean_match = MEAN_PATTERN.search(completed.stdout)
    if not mean_match:
        print(completed.stdout[-1500:])
        print("  no in-level mean in the output")
        return None

    result = {
        "label": label,
        "fps": float(mean_match.group(2)),
        "update_ms": float(mean_match.group(3)),
        "draw_ms": float(mean_match.group(4)),
        "sections": parse_sections(completed.stdout),
    }
    print(f"  fps {result['fps']:.2f} | draw {result['draw_ms']:.2f} ms | update {result['update_ms']:.2f} ms")
    return result


SECTIONS_OF_INTEREST = (
    "game clear targets",
    "clear level targets",
    "background layers",
    "atmosphere resolve",
    "foreground layers",
    "lighting",
    "post lighting layers",
    "level draw",
    "TOTAL",
)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("nro_a")
    parser.add_argument("nro_b")
    parser.add_argument("--label-a", default=None)
    parser.add_argument("--label-b", default=None)
    parser.add_argument("--sample-seconds", type=float, default=45.0)
    parser.add_argument("--monitor", type=int, default=None)
    parser.add_argument("--rounds", type=int, default=1, help="how many times to run the both-orders pair")
    args = parser.parse_args()

    label_a = args.label_a or Path(args.nro_a).stem
    label_b = args.label_b or Path(args.nro_b).stem
    builds = {label_a: Path(args.nro_a), label_b: Path(args.nro_b)}

    order = [label_a, label_b, label_b, label_a] * args.rounds
    results = [run_once(label, builds[label], args.sample_seconds, args.monitor) for label in order]
    results = [result for result in results if result]

    print("\n--- per run ---")
    for result in results:
        print(f"  {result['label']:24s} fps {result['fps']:7.2f} | draw {result['draw_ms']:6.2f} ms")

    print("\n--- pooled, median of both orders ---")
    medians = {}
    for label in (label_a, label_b):
        group = [result for result in results if result["label"] == label]
        if not group:
            print(f"  {label}: no result")
            continue
        medians[label] = {
            "fps": statistics.median(result["fps"] for result in group),
            "draw_ms": statistics.median(result["draw_ms"] for result in group),
            "runs": len(group),
        }
        print(
            f"  {label:24s} n={medians[label]['runs']} fps {medians[label]['fps']:7.2f}"
            f" (hardware guess {medians[label]['fps'] / 3.0:5.2f}) | draw {medians[label]['draw_ms']:6.2f} ms"
        )

    if len(medians) == 2:
        speedup = medians[label_a]["draw_ms"] / medians[label_b]["draw_ms"]
        print(f"\n  {label_b} draw is {speedup:.3f}x {label_a}")

    print("\n--- sections, median ms over the runs of each build ---")
    print(f"  {'section':22s} {label_a:>12s} {label_b:>12s}   delta")
    for name in SECTIONS_OF_INTEREST:
        values = {}
        for label in (label_a, label_b):
            samples = [result["sections"][name] for result in results if result["label"] == label and name in result["sections"]]
            if samples:
                values[label] = statistics.median(samples)
        if len(values) == 2:
            delta = values[label_b] - values[label_a]
            print(f"  {name:22s} {values[label_a]:12.3f} {values[label_b]:12.3f}  {delta:+7.3f}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
