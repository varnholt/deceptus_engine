"""Structural validation of the Nintendo Switch build artifacts.

What this can and cannot do: there is no Switch and no emulator in this environment, so
nothing here proves the game *runs*. These checks prove the .nro is a well-formed homebrew
binary for the right architecture, that the Switch backends were actually linked in rather
than silently replaced by SDL's dummies, that the assets are really embedded, and that no
symbol libnx cannot provide slipped into the link.

Run from the repo root after a Switch build:

    uv run --project tests pytest tests/switch/test_switch_build.py -v

Override the build directory with DECEPTUS_SWITCH_BUILD_DIR if it is not
build_switch_engine.
"""

import os
import struct
import subprocess
from pathlib import Path

import pytest

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
BUILD_DIRECTORY = Path(
    os.environ.get("DECEPTUS_SWITCH_BUILD_DIR", REPOSITORY_ROOT / "build_switch_engine")
)

NRO_PATH = BUILD_DIRECTORY / "deceptus.nro"
ELF_PATH = BUILD_DIRECTORY / "deceptus.elf"
ROMFS_PATH = BUILD_DIRECTORY / "romfs"

# same pinned tag build_switch.bat and the CI workflow use, so a developer machine is never
# asked to pull a second copy of a 2.8 GB image just to read a symbol table
DOCKER_IMAGE = "devkitpro/devkita64:20260219"
NM_BINARY = "/opt/devkitpro/devkitA64/bin/aarch64-none-elf-nm"

# assets the game reaches for by hard-coded relative path; if romfs does not carry these,
# it will fail at runtime no matter how cleanly it linked
REQUIRED_ASSETS = [
    "data/fonts/deceptum.ttf",
    "data/config",
    "data/scripts",
    "data/sprites",
    "data/shaders",
]


def _require(path: Path):
    if not path.exists():
        pytest.skip(f"{path} not found - run a Switch build first")
    return path


def _read_elf_symbols() -> str:
    """Symbol table of the linked ELF.

    Uses the devkitPro nm directly whenever the toolchain is present, which is the case when
    these tests run inside the container -- CI does exactly that. Only when it is absent does
    this reach for the container through docker, which is how it runs on a developer machine,
    where devkitPro is deliberately not installed natively.

    The distinction matters: without the local path, every symbol-based test would skip in CI
    rather than fail, and those are precisely the checks that catch SDL silently substituting
    its dummy backends for a broken port.
    """
    if Path(NM_BINARY).exists():
        command = [NM_BINARY, str(ELF_PATH)]
        failure_hint = "could not read symbols with the local devkitPro nm"
    else:
        command = [
            "docker",
            "run",
            "--rm",
            "-v",
            f"{REPOSITORY_ROOT}:/workspace",
            "-w",
            "/workspace",
            DOCKER_IMAGE,
            NM_BINARY,
            ELF_PATH.relative_to(REPOSITORY_ROOT).as_posix(),
        ]
        failure_hint = "could not read symbols via docker"

    completed = subprocess.run(
        command,
        capture_output=True,
        text=True,
        env={**os.environ, "MSYS_NO_PATHCONV": "1"},
    )
    if completed.returncode != 0:
        pytest.skip(f"{failure_hint}: {completed.stderr[:200]}")
    return completed.stdout


@pytest.fixture(scope="module")
def elf_symbols() -> set:
    """Symbol names only.

    Returning a set rather than the raw nm output keeps failures readable -- asserting
    against the whole blob makes pytest dump a multi-megabyte symbol table.
    """
    _require(ELF_PATH)
    names = set()
    for line in _read_elf_symbols().splitlines():
        parts = line.split()
        if len(parts) >= 2:
            names.add(parts[-1])
    return names


class TestNroContainer:
    """The .nro has to be a valid homebrew container before anything else matters."""

    def test_nro_exists(self):
        _require(NRO_PATH)

    def test_nro_magic(self):
        """elf2nro writes the ASCII tag 'NRO0' at offset 0x10."""
        _require(NRO_PATH)
        with NRO_PATH.open("rb") as handle:
            handle.seek(0x10)
            assert handle.read(4) == b"NRO0"

    def test_nro_declared_size_matches_file(self):
        """The size field at 0x18 must agree with the file, or the loader rejects it.

        A romfs is appended past the NRO image, so the file is allowed to be larger than
        the declared size -- but never smaller.
        """
        _require(NRO_PATH)
        with NRO_PATH.open("rb") as handle:
            handle.seek(0x18)
            declared_size = struct.unpack("<I", handle.read(4))[0]
        assert declared_size > 0
        assert NRO_PATH.stat().st_size >= declared_size

    def test_nro_carries_romfs(self):
        """With assets embedded the file must be substantially bigger than the code."""
        _require(NRO_PATH)
        with NRO_PATH.open("rb") as handle:
            handle.seek(0x18)
            declared_size = struct.unpack("<I", handle.read(4))[0]
        appended_bytes = NRO_PATH.stat().st_size - declared_size
        assert appended_bytes > 50 * 1024 * 1024, (
            f"only {appended_bytes} bytes appended past the NRO image; "
            "romfs looks missing or truncated"
        )


class TestArchitecture:
    def test_elf_is_aarch64_pie(self):
        _require(ELF_PATH)
        with ELF_PATH.open("rb") as handle:
            header = handle.read(20)
        assert header[:4] == b"\x7fELF"
        assert header[4] == 2, "expected 64-bit ELF"
        assert header[5] == 1, "expected little endian"
        elf_type = struct.unpack("<H", header[16:18])[0]
        machine = struct.unpack("<H", header[18:20])[0]
        assert elf_type == 3, "expected a position-independent executable"
        assert machine == 0xB7, "expected AArch64"


class TestSwitchBackendsLinked:
    """SDL silently falls back to dummy drivers, so absence of these would be invisible."""

    @pytest.mark.parametrize(
        "symbol",
        [
            "SWITCH_VideoInit",
            "SWITCH_CreateWindow",
            "SWITCH_PumpEvents",
            "SWITCH_GLES_CreateContext",
            "SDL_SWITCH_JoystickDriver",
        ],
    )
    def test_backend_symbol_present(self, elf_symbols, symbol):
        assert symbol in elf_symbols, f"{symbol} missing - the Switch backend was not linked in"

    def test_libnx_entry_points_used(self, elf_symbols):
        """The video backend and main loop must actually be talking to libnx.

        romfsInit() is deliberately not checked here: libnx defines it as an inline
        wrapper, so it never appears as a symbol. The mount machinery it pulls in is
        the observable evidence instead - see test_romfs_mounted_at_runtime.
        """
        for symbol in ("nwindowGetDefault", "appletMainLoop"):
            assert symbol in elf_symbols, f"{symbol} not referenced"

    def test_romfs_mounted_at_runtime(self, elf_symbols):
        """main() calls romfsInit(); these are what that drags in."""
        for symbol in ("romFS_devoptab", "romfsFindMount", "romfs_chdir"):
            assert symbol in elf_symbols, f"{symbol} missing - romfs is never mounted"

    def test_audio_goes_through_audout_not_sdl(self, elf_symbols):
        """Sound comes from miniaudio's custom backend over libnx audout.

        VRSFML sets SDL_AUDIO OFF and brings its own miniaudio, so SDL's audio subsystem --
        including the Switch audio backend in switch-sdl3-backend.patch -- is not compiled into
        the game at all. That backend stays in the patch because it makes SDL's own audio work
        for other consumers, but it is not what produces sound here.

        The audout symbols are the load-bearing half. Without them miniaudio finds no backend on
        the console and settles on ma_backend_null, which initializes cleanly, reports a device
        and plays absolutely nothing -- which is exactly how the port shipped silent, with no
        error in any log to give it away. A silent regression here would be invisible at runtime,
        so it is asserted at link time instead.
        """
        assert "SWITCHAUD_bootstrap" not in elf_symbols

        for symbol in ("audoutInitialize", "audoutAppendAudioOutBuffer", "audoutWaitPlayFinish"):
            assert symbol in elf_symbols, f"{symbol} missing - audio would fall back to the silent null backend"


class TestNoUnsupportedSymbols:
    """libnx provides no process control; these would only appear via an unguarded path."""

    @pytest.mark.parametrize("symbol", ["execvp", "waitpid"])
    def test_process_symbol_absent(self, elf_symbols, symbol):
        """These come in through upstream ImGui's "open in shell" path, which is why it
        is excluded from this build; a fully linked ELF could not contain them anyway,
        so this guards against the exclusion being undone."""
        assert symbol not in elf_symbols, f"{symbol} referenced - libnx cannot provide it"

    def test_imgui_not_linked(self, elf_symbols):
        """ImGui is deliberately excluded from the Switch build."""
        assert "ImGui_ImplOpenGL" not in elf_symbols
        assert "igNewFrame" not in elf_symbols


class TestRomfsStaging:
    """The staged tree is what elf2nro embeds, so validate its shape directly."""

    def test_romfs_directory_exists(self):
        _require(ROMFS_PATH)

    def test_data_is_nested_under_romfs(self):
        """The engine uses relative paths like "data/sprites/x.png".

        That means romfs must contain a 'data' directory, not the contents of data/ at
        its root -- getting this wrong is silent until the game cannot find a texture.
        """
        _require(ROMFS_PATH)
        assert (ROMFS_PATH / "data").is_dir(), "romfs must carry a nested data/ directory"

    @pytest.mark.parametrize("relative_path", REQUIRED_ASSETS)
    def test_required_asset_present(self, relative_path):
        _require(ROMFS_PATH)
        assert (ROMFS_PATH / relative_path).exists(), f"{relative_path} missing from romfs"

    def test_romfs_matches_source_data(self):
        """Staging is incremental, so a stale romfs would silently ship old assets."""
        _require(ROMFS_PATH)
        source_files = {
            path.relative_to(REPOSITORY_ROOT / "data")
            for path in (REPOSITORY_ROOT / "data").rglob("*")
            if path.is_file()
        }
        staged_files = {
            path.relative_to(ROMFS_PATH / "data")
            for path in (ROMFS_PATH / "data").rglob("*")
            if path.is_file()
        }
        missing = source_files - staged_files
        assert not missing, f"{len(missing)} file(s) missing from romfs, e.g. {sorted(missing)[:5]}"
