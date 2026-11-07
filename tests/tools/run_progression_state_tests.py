"""Run focused state tests against an existing Windows MSVC/Ninja desktop build.

Run from an x64 Visual Studio developer prompt after building the game:
    python tests/tools/run_progression_state_tests.py [build_rel]

Reuses the engine's compiled objects, replacing its entry point with the test driver.
The driver runs Lua regression fixtures and exercises real mechanism serialization.
"""

from pathlib import Path
import re
import subprocess
import sys


def main():
    root = Path(__file__).resolve().parents[2]
    build = (root / (sys.argv[1] if len(sys.argv) > 1 else "build_rel")).resolve()
    cache = (build / "CMakeCache.txt").read_text()
    ninja = re.search(r"^CMAKE_MAKE_PROGRAM:FILEPATH=(.+)$", cache, re.M).group(1)
    source = root / "tests/desktop/progression_state.cpp"
    obj = "progression_state_test.obj"
    exe = build / "progression_state_test.exe"

    command = subprocess.check_output(
        [ninja, "-C", str(build), "-t", "commands",
         "CMakeFiles/deceptus.dir/src/game/mechanisms/infooverlay.cpp.obj"],
        text=True,
    ).splitlines()[-1]
    command = command.replace(
        str(root / "src/game/mechanisms/infooverlay.cpp"),
        subprocess.list2cmdline([str(source)]),
    )
    command = re.sub(r"/Fo\S+", "/Fo" + obj, command)
    command = re.sub(r"/Fd\S+", "/Fdprogression_state_test.pdb", command)
    command = command.replace("/showIncludes", "")
    subprocess.run(command, cwd=build, shell=True, check=True)

    lines = (build / "build.ninja").read_text().splitlines()
    index = next(i for i, line in enumerate(lines) if line.startswith("build deceptus.exe:"))
    objects = lines[index].split(": CXX_EXECUTABLE_LINKER__deceptus_", 1)[1]
    objects = objects.split(" ", 1)[1].split(" | ", 1)[0].split()
    main_obj = r"CMakeFiles\deceptus.dir\src\main.cpp.obj"
    if main_obj not in objects:
        raise RuntimeError("cannot find the engine entry point in the desktop build")
    objects = [obj if item == main_obj else item for item in objects]
    libraries = next(
        line.split(" = ", 1)[1] for line in lines[index + 1:]
        if line.startswith("  LINK_LIBRARIES =")
    )
    rsp = build / "progression_state_test.rsp"
    rsp.write_text("\n".join(objects) + "\n" + libraries)
    subprocess.run(
        ["link.exe", "/nologo", "@" + rsp.name, "/out:" + exe.name,
         "/subsystem:console", "/machine:x64", "/incremental:no"],
        cwd=build, check=True,
    )
    subprocess.run([str(exe)], cwd=root, check=True)


if __name__ == "__main__":
    main()
