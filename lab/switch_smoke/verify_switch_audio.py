"""Proves the Switch build actually makes a sound, rather than merely claiming to.

    uv run --with soundcard python verify_switch_audio.py

The engine logging "Audio playing through the switch audout backend" only says the backend was
selected. It says nothing about whether samples reach the speakers, which is exactly the failure
this whole exercise is about: miniaudio's null backend also initializes perfectly and also plays
nothing. So this listens instead of reading logs.

It captures the host's own output through a WASAPI loopback device while Ryujinx runs the .nro, and
compares the level against a baseline recorded before the emulator starts. Anything else playing on
the machine lands in the same recording, hence the baseline: it establishes what "quiet" is right
now instead of assuming zero.

Note this can only be as honest as the host's mixer. A muted or silenced output device produces a
flat loopback stream no matter what the guest submits, so a failure here means "no sound reached the
speakers", which may still be the host's fault rather than the guest's.
"""

import sys
import time
from pathlib import Path

import numpy
import soundcard

sys.path.insert(0, str(Path(__file__).resolve().parent))

from ryujinx_driver import NRO_PATH, RyujinxSession, send_key, VK_X  # noqa: E402

SAMPLE_RATE = 48000

# a loopback stream of a genuinely silent output sits at or below this; music and effects are orders
# of magnitude above it
SILENCE_RMS = 1e-4


def loopback_microphone():
    for microphone in soundcard.all_microphones(include_loopback=True):
        if microphone.isloopback and microphone.name == soundcard.default_speaker().name:
            return microphone
    raise RuntimeError("no loopback device matching the default speaker")


def measure_rms(microphone, seconds: float, label: str) -> float:
    frames = int(SAMPLE_RATE * seconds)
    with microphone.recorder(samplerate=SAMPLE_RATE, channels=2) as recorder:
        recording = recorder.record(numframes=frames)

    rms = float(numpy.sqrt(numpy.mean(numpy.square(recording))))
    peak = float(numpy.max(numpy.abs(recording)))
    print(f"{label:<28} rms {rms:.6f}   peak {peak:.6f}")
    return rms


def main() -> int:
    microphone = loopback_microphone()
    print(f"listening on {microphone.name}\n")

    baseline_rms = measure_rms(microphone, 3.0, "baseline (nothing running)")

    with RyujinxSession(nro_path=NRO_PATH):
        time.sleep(50)
        menu_rms = measure_rms(microphone, 6.0, "main menu")

        send_key(VK_X, settle_s=3.0)
        send_key(VK_X, settle_s=3.0)
        time.sleep(35)
        ingame_rms = measure_rms(microphone, 8.0, "in the level")

    loudest = max(menu_rms, ingame_rms)
    print()

    if baseline_rms > SILENCE_RMS:
        print(f"inconclusive: the machine was already making noise before launch (rms {baseline_rms:.6f})")
        return 2

    if loudest <= SILENCE_RMS:
        print(f"NO AUDIO: loudest sample was {loudest:.6f}, indistinguishable from silence")
        return 1

    print(f"AUDIO CONFIRMED: {loudest / max(baseline_rms, 1e-9):.0f}x above the silent baseline")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
