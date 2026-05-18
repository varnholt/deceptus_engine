"""
Compare the standalone path_merge binary against the Qt5 reference binary.

Both binaries accept: path_merge.exe <input.obj> <output.obj>

Test strategy:
  - Run each binary on each *_not_optimised.obj input.
  - Assert the outputs are identical after trimming trailing whitespace per line.
"""

import re
import subprocess
from pathlib import Path

import pytest

HERE = Path(__file__).parent
OUTPUT_DIR = HERE / "test_output"

BINARY_QT5 = HERE / "binary_qt5" / "path_merge.exe"
BINARY_STANDALONE = HERE / "binary_standalone" / "path_merge.exe"

TEST_INPUTS = [
    HERE / "test_data" / "layer_level_solid_not_optimised.obj",
    HERE / "test_data" / "layer_level_solid_onesided_solid_onesided_not_optimised.obj",
]


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


_STATS_RE = re.compile(r"points: (\d+) -> (\d+), faces: (\d+) -> (\d+), factor: ([\d.]+)")


def normalized_lines(path: Path) -> list[str]:
    """Return non-empty lines with trailing whitespace stripped."""
    return [line.rstrip() for line in path.read_text().splitlines() if line.strip()]


def run_binary(binary: Path, input_obj: Path, output_obj: Path) -> dict:
    result = subprocess.run(
        [str(binary), str(input_obj), str(output_obj)],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, (
        f"{binary.name} failed on {input_obj.name}:\n{result.stdout}\n{result.stderr}"
    )
    match = _STATS_RE.search(result.stdout)
    assert match, f"could not parse stats from {binary.name} output:\n{result.stdout}"
    return {
        "points_in":  int(match.group(1)),
        "points_out": int(match.group(2)),
        "faces_in":   int(match.group(3)),
        "faces_out":  int(match.group(4)),
        "factor":     match.group(5),
    }


# ---------------------------------------------------------------------------
# Parametrized tests
# ---------------------------------------------------------------------------


@pytest.mark.parametrize("input_obj", TEST_INPUTS, ids=lambda p: p.stem)
def test_standalone_matches_qt5(input_obj: Path) -> None:
    """Standalone output must be line-for-line identical to Qt5 output (ignoring trailing whitespace)."""
    OUTPUT_DIR.mkdir(exist_ok=True)
    base_stem = input_obj.stem.replace("_not_optimised", "")
    output_qt5 = OUTPUT_DIR / f"qt5_{base_stem}.obj"
    output_standalone = OUTPUT_DIR / f"standalone_{base_stem}.obj"

    stats_qt5 = run_binary(BINARY_QT5, input_obj, output_qt5)
    stats_standalone = run_binary(BINARY_STANDALONE, input_obj, output_standalone)

    lines_qt5 = normalized_lines(output_qt5)
    lines_standalone = normalized_lines(output_standalone)

    print(f"\n  {'input:':<12} {input_obj.name}")
    print(f"  {'points:':<12} {stats_qt5['points_in']} -> {stats_qt5['points_out']}")
    print(f"  {'faces:':<12} {stats_qt5['faces_in']} -> {stats_qt5['faces_out']}")
    print(f"  {'factor:':<12} qt5={stats_qt5['factor']}  standalone={stats_standalone['factor']}")
    print(f"  {'lines:':<12} qt5={len(lines_qt5)}  standalone={len(lines_standalone)}")
    print(f"  {'qt5:':<12} {output_qt5.name}")
    print(f"  {'standalone:':<12} {output_standalone.name}")

    assert lines_standalone == lines_qt5


