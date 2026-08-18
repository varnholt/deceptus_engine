"""Records the guest's audio and characterises *how* it is broken, not merely that it is audible.

    uv run --with soundcard --with numpy python analyze_switch_audio.py [seconds]

verify_switch_audio.py only measures level, which a completely garbled stream passes just as
happily as a clean one. This looks at the shape of the signal instead, because the three ways this
backend can go wrong leave different fingerprints:

- dropouts: buffers arriving late, so the sink runs dry between them. Shows up as short stretches of
  near-silence punctuated by sound, at a rate tied to the period size.
- repeats: a buffer being overwritten or replayed while the console still owns it. Shows up as high
  correlation at a lag equal to one period.
- format or rate mismatch: interpreting the samples wrongly. Shows up as energy piled into the wrong
  part of the spectrum, typically much lower than music should sit.

The recording is written to out/switch_audio.wav so it can be listened to directly.
"""

import sys
import time
import wave
from pathlib import Path

import numpy
import soundcard

sys.path.insert(0, str(Path(__file__).resolve().parent))

from ryujinx_driver import NRO_PATH, RyujinxSession, send_key, VK_X  # noqa: E402

SAMPLE_RATE = 48000
OUTPUT_PATH = Path(__file__).resolve().parent / "out" / "switch_audio.wav"


def loopback_microphone():
    for microphone in soundcard.all_microphones(include_loopback=True):
        if microphone.isloopback and microphone.name == soundcard.default_speaker().name:
            return microphone
    raise RuntimeError("no loopback device matching the default speaker")


def record(seconds: float) -> numpy.ndarray:
    microphone = loopback_microphone()
    print(f"recording {seconds:.0f}s from {microphone.name}")
    with microphone.recorder(samplerate=SAMPLE_RATE, channels=2) as recorder:
        return recorder.record(numframes=int(SAMPLE_RATE * seconds))


def save_wav(recording: numpy.ndarray) -> None:
    OUTPUT_PATH.parent.mkdir(exist_ok=True)
    samples = numpy.clip(recording, -1.0, 1.0)
    with wave.open(str(OUTPUT_PATH), "wb") as handle:
        handle.setnchannels(2)
        handle.setsampwidth(2)
        handle.setframerate(SAMPLE_RATE)
        handle.writeframes((samples * 32767).astype("<i2").tobytes())
    print(f"wrote {OUTPUT_PATH}")


def describe(recording: numpy.ndarray) -> None:
    mono = recording.mean(axis=1)

    # short-term level, at a resolution fine enough to see a dropped period
    window = 256
    usable = len(mono) - (len(mono) % window)
    envelope = numpy.sqrt(numpy.mean(numpy.square(mono[:usable].reshape(-1, window)), axis=1))

    overall = float(numpy.sqrt(numpy.mean(numpy.square(mono))))
    print(f"\noverall rms      {overall:.6f}")

    if overall <= 1e-6:
        print("signal is silent, nothing to characterise")
        return

    # a clean music stream rarely drops below a hundredth of its own average; a starved sink does it
    # constantly, once per missed period
    quiet = envelope < (overall * 0.05)
    quiet_fraction = float(numpy.mean(quiet))
    print(f"near-silent time {quiet_fraction * 100:.1f}%   ({window / SAMPLE_RATE * 1000:.1f} ms resolution)")

    # count how often it crosses into near-silence: dropouts are many short crossings, a genuine
    # musical rest is one long one
    crossings = int(numpy.sum(numpy.diff(quiet.astype(numpy.int8)) == 1))
    seconds = len(mono) / SAMPLE_RATE
    print(f"dropout events   {crossings}  ({crossings / seconds:.1f} per second)")

    # spectral centre of mass says whether the energy sits where music lives
    spectrum = numpy.abs(numpy.fft.rfft(mono * numpy.hanning(len(mono))))
    frequencies = numpy.fft.rfftfreq(len(mono), 1.0 / SAMPLE_RATE)
    centroid = float(numpy.sum(spectrum * frequencies) / numpy.sum(spectrum))
    peak = float(frequencies[int(numpy.argmax(spectrum))])
    print(f"spectral centroid {centroid:.0f} Hz   peak {peak:.0f} Hz")


def main() -> int:
    arguments = [argument for argument in sys.argv[1:] if not argument.startswith("--")]
    seconds = float(arguments[0]) if arguments else 8.0

    # --no-launch records whatever is already playing, which is how the desktop build is measured as
    # a control: without knowing what these numbers look like on audio that is known to be correct,
    # a dropout count says nothing at all
    if "--no-launch" in sys.argv:
        recording = record(seconds)
        save_wav(recording)
        describe(recording)
        return 0

    # the emulator is launched from this process rather than alongside it: started from a detached
    # background job its audio never reached the loopback device and every recording came back
    # silent, which looks exactly like a broken backend and is not
    with RyujinxSession(nro_path=NRO_PATH):
        time.sleep(50)
        print("\n--- main menu (music) ---")
        menu_recording = record(seconds)
        save_wav(menu_recording)
        describe(menu_recording)

        send_key(VK_X, settle_s=3.0)
        send_key(VK_X, settle_s=3.0)
        time.sleep(35)
        print("\n--- in the level (music and effects) ---")
        describe(record(seconds))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
