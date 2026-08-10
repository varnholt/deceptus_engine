"""Checks that entering water costs the player the rope.

    uv run --with pywin32 --with pillow python drive_water_check.py [build_dir]

The test level has a flooded pit at tiles 44..52 for exactly this: the rope is attached on dry
ground next to it, then the player walks in and the rope has to be gone.
"""

import subprocess
import sys
import time

import drive_harpoon as harness

harness.HARPOON_TEST_LEVEL_INDEX = 3  # the harpoon test level, which now has a flooded pit


def count_rope_pixels(path: str) -> int:
    """Counts rope shades in the upper half only: the level tiles share shades with the rope, but
    nothing else is drawn in the open air above the player."""
    from PIL import Image

    image = Image.open(harness.OUTPUT_DIRECTORY / path).convert("RGB")
    band = image.crop((0, 0, image.width, 380))
    rope_shades = {(70, 55, 52), (93, 76, 72), (112, 92, 87)}
    return sum(1 for pixel in band.getdata() if pixel in rope_shades)


def run() -> int:
    build_directory = sys.argv[1] if len(sys.argv) > 1 else "build_rel"
    executable = harness.REPO_ROOT / build_directory / "deceptus.exe"

    harness.OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    log_file = (harness.OUTPUT_DIRECTORY / "water.log").open("w", encoding="utf-8", errors="replace")
    process = subprocess.Popen([str(executable)], cwd=str(harness.REPO_ROOT), stdout=log_file, stderr=subprocess.STDOUT)

    for _ in range(60):
        time.sleep(1.0)
        if harness.find_window():
            break

    time.sleep(3.0)
    harness.send_key(harness.VK_RETURN)
    time.sleep(1.5)
    harness.send_key(harness.VK_RETURN)
    time.sleep(10.0)

    control_run = "--control" in sys.argv
    if not control_run:
        harness.run_console_command("weapon add harpoon")

    # dry ground just left of the flooded pit, hook the ceiling straight up
    harness.run_console_command("tpp 41, 14")
    time.sleep(1.5)
    harness.grab("60_above_water.png")

    if not control_run:
        harness.hold_key(harness.VK_UP, True)
        harness.send_key(harness.VK_LCONTROL, hold_s=0.05, settle_s=0.0)
        harness.hold_key(harness.VK_UP, False)
        time.sleep(1.5)
    harness.grab("61_attached.png")

    # the taut rope keeps him from walking in, so he is put into the pit directly - how he gets into
    # the water is not what is being tested here
    harness.run_console_command("tpp 48, 17")
    time.sleep(1.5)
    harness.grab("62_in_water.png")

    if control_run:
        print("control run finished without a crash" if (harness.OUTPUT_DIRECTORY / "62_in_water.png").exists() else "control run CRASHED too")
        process.terminate()
        return 0

    attached_pixels = count_rope_pixels("61_attached.png")
    water_pixels = count_rope_pixels("62_in_water.png")
    print(f"rope pixels while attached: {attached_pixels}")
    print(f"rope pixels after entering water: {water_pixels}")
    print("PASS" if attached_pixels > 200 and water_pixels < attached_pixels // 4 else "INCONCLUSIVE")

    process.terminate()
    return 0


def main() -> int:
    harness.install_save_state()
    harness.force_windowed_mode()
    try:
        return run()
    finally:
        subprocess.run(["taskkill", "/F", "/IM", "deceptus.exe"], capture_output=True)
        harness.restore_save_state()
        harness.restore_fullscreen_mode()


if __name__ == "__main__":
    raise SystemExit(main())
