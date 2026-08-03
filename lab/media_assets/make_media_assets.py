"""Derive every publishable media asset from one lossless gameplay master.

`lab/record_gameplay` records `output/master.mkv`, a lossless capture of the game window.
This script turns that master into the assets the project actually publishes:

    stills/still_<seconds>.png  full resolution PNG frames, for the README hero image,
                                itch.io screenshots, store pages and bug reports
    thumbnail.png               640x360, the engine's native framebuffer size
    gameplay.mp4                H.264, full resolution, the best quality per byte by far
    gameplay.webm               VP9, for pages that prefer it
    gameplay.gif                animation for the README, kept under a size budget
    gameplay_itch.gif           animation for itch.io, which accepts images only and caps
                                uploads at 3 MB

Only the assets that end up referenced by README.md need to be committed; everything else can
stay in the ignored output directory. `--install` copies the committed subset into
doc/screenshots/.

Usage:
    uv run --project lab/media_assets lab/media_assets/make_media_assets.py
    uv run --project lab/media_assets lab/media_assets/make_media_assets.py --help
"""

import argparse
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_INPUT_PATH = REPO_ROOT / "lab" / "record_gameplay" / "output" / "master.mkv"
DEFAULT_OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "output"
INSTALL_DIRECTORY = REPO_ROOT / "doc" / "screenshots"
BYTES_PER_MEGABYTE = 1024 * 1024

# record_gameplay captures the client area only, so a master needs no cropping. --crop stays
# available for sources that do carry window chrome, for example an older capture.
DEFAULT_CROP = "none"

# The engine rasterises into a render texture of size_ratio * view, where the ratio is the
# integer part of window / view (see Game::initializeRenderTargets). At a 1280x720 window and a
# 640x360 view that is a full 1280x720 target: tile art is drawn at 2x, but text, HUD filigree
# and every shader rasterise at the full resolution. Downscaling to 640x360 keeps the pixel art
# intact and throws the shader and text detail away, so it is a fallback, not the default.
NATIVE_VIEW_WIDTH = 640

# GIF frame durations are stored in hundredths of a second, so only rates that divide evenly
# into 100 play back at a stable speed.
VALID_GIF_FRAME_RATES: tuple[int, ...] = (50, 25, 20, 10, 5)

README_GIF_BUDGET_MEGABYTES = 8.0
ITCH_GIF_BUDGET_MEGABYTES = 3.0  # itch.io's per-image upload limit

# What README.md references, copied into doc/screenshots/ by --install. Keep this in sync with
# the README; everything else belongs in the ignored output directory.
INSTALLED_ASSETS: tuple[str, ...] = ("screenshot.png", "gameplay.gif", "gameplay.mp4")


@dataclass(frozen=True)
class QualityStep:
    """One rung of the GIF quality ladder."""

    width: int | None  # output width in pixels, None keeps the source width
    frames_per_second: int  # GIF frame rate, must divide evenly into 100
    max_colors: int  # palette size handed to ffmpeg palettegen

    def describe(self) -> str:
        width_text = "native" if self.width is None else f"{self.width}px"
        return f"{width_text} @ {self.frames_per_second} fps, {self.max_colors} colors"


def format_size(path: Path) -> str:
    return f"{path.stat().st_size / BYTES_PER_MEGABYTE:.2f} MB"


def run_ffmpeg(arguments: list[str], description: str) -> None:
    completed_process = subprocess.run(
        ["ffmpeg", "-y", "-hide_banner", "-loglevel", "error", *arguments],
        capture_output=True,
        text=True,
    )
    if completed_process.returncode != 0:
        raise SystemExit(f"{description} failed:\n{completed_process.stderr}")


def probe_dimensions(path: Path) -> tuple[int, int]:
    completed_process = subprocess.run(
        [
            "ffprobe",
            "-v",
            "error",
            "-select_streams",
            "v:0",
            "-show_entries",
            "stream=width,height",
            "-of",
            "csv=p=0",
            str(path),
        ],
        capture_output=True,
        text=True,
    )
    if completed_process.returncode != 0:
        raise SystemExit(f"ffprobe failed on {path}:\n{completed_process.stderr}")
    width_text, height_text = completed_process.stdout.strip().splitlines()[0].split(",")[:2]
    return int(width_text), int(height_text)


def resolve_crop(crop: str | None, input_path: Path) -> str | None:
    """Drop the crop when the source is too small for it.

    The default crop describes a raw window capture. Running the script on an already cropped
    file, such as one of its own outputs, would otherwise fail.
    """
    if crop is None:
        return None

    crop_fields = crop.split(":")
    if len(crop_fields) < 2 or not all(field.lstrip("-").isdigit() for field in crop_fields[:2]):
        return crop

    crop_width = int(crop_fields[0])
    crop_height = int(crop_fields[1])
    source_width, source_height = probe_dimensions(input_path)
    if crop_width > source_width or crop_height > source_height:
        print(f"note: source is {source_width}x{source_height}, skipping the {crop_width}x{crop_height} crop")
        return None
    return crop


def build_trim_arguments(start_seconds: float | None, duration_seconds: float | None) -> list[str]:
    trim_arguments: list[str] = []
    if start_seconds is not None:
        trim_arguments += ["-ss", str(start_seconds)]
    if duration_seconds is not None:
        trim_arguments += ["-t", str(duration_seconds)]
    return trim_arguments


def build_source_filter(crop: str | None, width: int | None, scale_flags: str) -> str:
    """Assemble the shared crop and scale part of the filter graph."""
    filter_steps: list[str] = []
    if crop is not None:
        filter_steps.append(f"crop={crop}")
    if width is not None:
        filter_steps.append(f"scale={width}:-2:flags={scale_flags}")
    return ",".join(filter_steps) if filter_steps else "null"


def next_lower_gif_frame_rate(frames_per_second: int) -> int:
    """Return the highest valid GIF rate that is at most half of the given one."""
    candidates = [rate for rate in VALID_GIF_FRAME_RATES if rate <= frames_per_second / 2]
    return max(candidates) if candidates else frames_per_second


def build_quality_ladder(frames_per_second: int, fallback_width: int, start_width: int | None) -> list[QualityStep]:
    """Build the descending list of GIF settings to try.

    Palette size goes first because it is the least visible, then the frame rate, and only when
    neither is enough does the resolution drop to the native view size. `start_width` of None
    means start at the source resolution.
    """
    halved_frame_rate = next_lower_gif_frame_rate(frames_per_second)

    quality_ladder = [QualityStep(start_width, frames_per_second, colors) for colors in (256, 192, 128)]
    if halved_frame_rate != frames_per_second:
        quality_ladder += [QualityStep(start_width, halved_frame_rate, colors) for colors in (192, 128)]
    if start_width == fallback_width:
        return quality_ladder
    quality_ladder += [QualityStep(fallback_width, frames_per_second, colors) for colors in (256, 192, 128)]
    if halved_frame_rate != frames_per_second:
        quality_ladder.append(QualityStep(fallback_width, halved_frame_rate, 128))
        quality_ladder.append(QualityStep(fallback_width, halved_frame_rate, 96))
        quartered_frame_rate = next_lower_gif_frame_rate(halved_frame_rate)
        if quartered_frame_rate != halved_frame_rate:
            quality_ladder.append(QualityStep(fallback_width, quartered_frame_rate, 96))
    return quality_ladder


def encode_still(
    input_path: Path,
    output_path: Path,
    timestamp_seconds: float,
    crop: str | None,
    width: int | None,
    scale_flags: str,
) -> None:
    run_ffmpeg(
        [
            "-ss",
            str(timestamp_seconds),
            "-i",
            str(input_path),
            "-vf",
            build_source_filter(crop, width, scale_flags),
            "-frames:v",
            "1",
            str(output_path),
        ],
        f"still at {timestamp_seconds}s",
    )


def encode_gif(
    input_path: Path,
    output_path: Path,
    quality_step: QualityStep,
    crop: str | None,
    scale_flags: str,
    dither: str,
    trim_arguments: list[str],
) -> None:
    """Encode a GIF using the split/palettegen/paletteuse graph.

    `stats_mode=diff` biases the palette towards what moves between frames instead of towards
    whole frames, and `diff_mode=rectangle` limits every written frame to the rectangle that
    actually changed.
    """
    source_filter = build_source_filter(crop, quality_step.width, scale_flags)
    filter_graph = (
        f"fps={quality_step.frames_per_second},{source_filter},split[palette_source][gif_source];"
        f"[palette_source]palettegen=max_colors={quality_step.max_colors}:stats_mode=diff[palette];"
        f"[gif_source][palette]paletteuse=dither={dither}:diff_mode=rectangle"
    )
    run_ffmpeg(
        [*trim_arguments, "-i", str(input_path), "-filter_complex", filter_graph, "-loop", "0", str(output_path)],
        "GIF encode",
    )


def encode_gif_within_budget(
    input_path: Path,
    output_path: Path,
    target_bytes: int,
    crop: str | None,
    scale_flags: str,
    dither: str,
    trim_arguments: list[str],
    frames_per_second: int,
    fallback_width: int,
    start_width: int | None,
) -> None:
    """Walk down the quality ladder until the GIF fits the size budget."""
    for quality_step in build_quality_ladder(frames_per_second, fallback_width, start_width):
        print(f"    {quality_step.describe()} ... ", end="", flush=True)
        encode_gif(input_path, output_path, quality_step, crop, scale_flags, dither, trim_arguments)
        print(format_size(output_path))
        if output_path.stat().st_size <= target_bytes:
            return

    print(f"    warning: could not reach {target_bytes / BYTES_PER_MEGABYTE:.2f} MB, keeping the smallest attempt")


def encode_video(
    input_path: Path,
    output_path: Path,
    codec_arguments: list[str],
    frames_per_second: int,
    crop: str | None,
    width: int | None,
    scale_flags: str,
    trim_arguments: list[str],
    description: str,
) -> None:
    source_filter = build_source_filter(crop, width, scale_flags)
    run_ffmpeg(
        [
            *trim_arguments,
            "-i",
            str(input_path),
            "-vf",
            f"fps={frames_per_second},{source_filter},format=yuv420p",
            *codec_arguments,
            "-an",
            str(output_path),
        ],
        description,
    )


def parse_still_timestamps(specification: str) -> list[float]:
    return [float(entry) for entry in specification.split(",") if entry.strip()]


def parse_arguments() -> argparse.Namespace:
    argument_parser = argparse.ArgumentParser(
        description="Derive stills, videos and GIFs from a lossless gameplay master."
    )
    argument_parser.add_argument(
        "--input",
        type=Path,
        default=DEFAULT_INPUT_PATH,
        help=f"lossless master recorded by lab/record_gameplay (default: {DEFAULT_INPUT_PATH})",
    )
    argument_parser.add_argument(
        "--output-directory",
        type=Path,
        default=DEFAULT_OUTPUT_DIRECTORY,
        help=f"where the assets are written (default: {DEFAULT_OUTPUT_DIRECTORY})",
    )
    argument_parser.add_argument(
        "--assets",
        default="stills,thumbnail,mp4,webm,gif,itch-gif",
        help="comma separated subset to produce (default: all of "
        "stills,thumbnail,mp4,webm,gif,itch-gif)",
    )
    argument_parser.add_argument(
        "--stills",
        default="1,2,3,4",
        help="timestamps in seconds to grab full resolution PNG frames at (default: 1,2,3,4)",
    )
    argument_parser.add_argument(
        "--hero-still",
        type=float,
        default=None,
        help="timestamp of the still that becomes screenshot.png, the README hero image "
        "(default: the first --stills entry)",
    )
    argument_parser.add_argument(
        "--fps",
        type=int,
        default=20,
        help="frame rate for animations, must divide evenly into 100 for GIF (default: 20)",
    )
    argument_parser.add_argument(
        "--start",
        type=float,
        default=None,
        help="trim start for the animations, in seconds",
    )
    argument_parser.add_argument(
        "--duration",
        type=float,
        default=None,
        help="animation length in seconds",
    )
    argument_parser.add_argument(
        "--crop",
        default=DEFAULT_CROP,
        help="ffmpeg crop expression (default: none). A master from record_gameplay is already "
        "the bare client area; use this for sources that include window chrome, e.g. 1280:720:8:31",
    )
    argument_parser.add_argument(
        "--scale-flags",
        default="area",
        help="ffmpeg scaler used when an asset has to be downscaled (default: area). Halving with "
        "area averages each 2x2 block, which returns the identical pixel for 2x scaled tile art "
        "while antialiasing text and shader output properly. neighbor picks one pixel per block "
        "instead, which aliases both",
    )
    argument_parser.add_argument(
        "--dither",
        default="none",
        help="ffmpeg paletteuse dither mode, dithering looks smoother but changes every pixel of "
        "every frame and so inflates GIFs badly (default: none)",
    )
    argument_parser.add_argument(
        "--gif-width",
        type=int,
        default=None,
        help="width the GIF ladder starts at (default: the source resolution). Set this when the "
        "clip is known to be too expensive at full resolution, for example a panning camera",
    )
    argument_parser.add_argument(
        "--gif-megabytes",
        type=float,
        default=README_GIF_BUDGET_MEGABYTES,
        help=f"size budget for gameplay.gif (default: {README_GIF_BUDGET_MEGABYTES})",
    )
    argument_parser.add_argument(
        "--itch-megabytes",
        type=float,
        default=ITCH_GIF_BUDGET_MEGABYTES,
        help=f"size budget for gameplay_itch.gif (default: {ITCH_GIF_BUDGET_MEGABYTES}, itch.io's limit)",
    )
    argument_parser.add_argument(
        "--video-crf",
        type=int,
        default=18,
        help="constant rate factor for MP4 and WebM, lower is better quality (default: 18)",
    )
    argument_parser.add_argument(
        "--install",
        action="store_true",
        help=f"copy the README referenced assets ({', '.join(INSTALLED_ASSETS)}) into doc/screenshots/",
    )
    return argument_parser.parse_args()


def main() -> int:
    arguments = parse_arguments()

    for executable in ("ffmpeg", "ffprobe"):
        if shutil.which(executable) is None:
            raise SystemExit(f"{executable} was not found on PATH")
    if not arguments.input.exists():
        raise SystemExit(
            f"input does not exist: {arguments.input}\n"
            f"record a master first: uv run --project lab/record_gameplay pytest lab/record_gameplay -s"
        )
    if "gif" in arguments.assets and arguments.fps not in VALID_GIF_FRAME_RATES:
        valid_rates = ", ".join(str(rate) for rate in VALID_GIF_FRAME_RATES)
        raise SystemExit(f"--fps {arguments.fps} does not divide evenly into 100, pick one of: {valid_rates}")

    requested_assets = [entry.strip().lower() for entry in arguments.assets.split(",") if entry.strip()]
    still_timestamps = parse_still_timestamps(arguments.stills)
    hero_timestamp = arguments.hero_still if arguments.hero_still is not None else still_timestamps[0]
    crop = resolve_crop(None if arguments.crop.lower() == "none" else arguments.crop, arguments.input)
    trim_arguments = build_trim_arguments(arguments.start, arguments.duration)
    output_directory = arguments.output_directory
    output_directory.mkdir(parents=True, exist_ok=True)

    print(f"master: {arguments.input} ({format_size(arguments.input)})")
    written_assets: list[Path] = []

    if "stills" in requested_assets:
        print("stills")
        stills_directory = output_directory / "stills"
        stills_directory.mkdir(exist_ok=True)
        for timestamp_seconds in still_timestamps:
            still_path = stills_directory / f"still_{timestamp_seconds:g}s.png"
            encode_still(arguments.input, still_path, timestamp_seconds, crop, None, arguments.scale_flags)
            print(f"    {still_path.name} ({format_size(still_path)})")
            written_assets.append(still_path)

        hero_path = output_directory / "screenshot.png"
        encode_still(arguments.input, hero_path, hero_timestamp, crop, None, arguments.scale_flags)
        print(f"    screenshot.png from {hero_timestamp:g}s ({format_size(hero_path)})")
        written_assets.append(hero_path)

    if "thumbnail" in requested_assets:
        thumbnail_path = output_directory / "thumbnail.png"
        encode_still(arguments.input, thumbnail_path, hero_timestamp, crop, NATIVE_VIEW_WIDTH, arguments.scale_flags)
        print(f"thumbnail.png ({format_size(thumbnail_path)})")
        written_assets.append(thumbnail_path)

    if "mp4" in requested_assets:
        mp4_path = output_directory / "gameplay.mp4"
        encode_video(
            arguments.input,
            mp4_path,
            ["-c:v", "libx264", "-preset", "veryslow", "-crf", str(arguments.video_crf), "-movflags", "+faststart"],
            arguments.fps,
            crop,
            None,
            arguments.scale_flags,
            trim_arguments,
            "MP4 encode",
        )
        print(f"gameplay.mp4 ({format_size(mp4_path)})")
        written_assets.append(mp4_path)

    if "webm" in requested_assets:
        webm_path = output_directory / "gameplay.webm"
        encode_video(
            arguments.input,
            webm_path,
            ["-c:v", "libvpx-vp9", "-crf", str(arguments.video_crf), "-b:v", "0", "-row-mt", "1"],
            arguments.fps,
            crop,
            None,
            arguments.scale_flags,
            trim_arguments,
            "WebM encode",
        )
        print(f"gameplay.webm ({format_size(webm_path)})")
        written_assets.append(webm_path)

    if "gif" in requested_assets:
        gif_path = output_directory / "gameplay.gif"
        print(f"gameplay.gif, budget {arguments.gif_megabytes:.2f} MB")
        encode_gif_within_budget(
            arguments.input,
            gif_path,
            int(arguments.gif_megabytes * BYTES_PER_MEGABYTE),
            crop,
            arguments.scale_flags,
            arguments.dither,
            trim_arguments,
            arguments.fps,
            NATIVE_VIEW_WIDTH,
            arguments.gif_width,
        )
        written_assets.append(gif_path)

    if "itch-gif" in requested_assets:
        itch_gif_path = output_directory / "gameplay_itch.gif"
        print(f"gameplay_itch.gif, budget {arguments.itch_megabytes:.2f} MB")
        encode_gif_within_budget(
            arguments.input,
            itch_gif_path,
            int(arguments.itch_megabytes * BYTES_PER_MEGABYTE),
            crop,
            arguments.scale_flags,
            arguments.dither,
            trim_arguments,
            arguments.fps,
            NATIVE_VIEW_WIDTH,
            arguments.gif_width,
        )
        written_assets.append(itch_gif_path)

    if arguments.install:
        print(f"installing into {INSTALL_DIRECTORY}")
        for asset_name in INSTALLED_ASSETS:
            source_path = output_directory / asset_name
            if not source_path.exists():
                print(f"    skipped {asset_name}, it was not produced")
                continue
            shutil.copyfile(source_path, INSTALL_DIRECTORY / asset_name)
            print(f"    {asset_name} ({format_size(source_path)})")

    print(f"\n{len(written_assets)} asset(s) in {output_directory}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
