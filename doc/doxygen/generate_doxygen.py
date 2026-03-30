# /// script
# requires-python = ">=3.9"
# ///

from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def build_doxyfile(repo_root: Path) -> str:
    src_dir = repo_root / "src"
    output_dir = repo_root / "doc"

    return "\n".join(
        [
            "PROJECT_NAME = deceptus",
            "OUTPUT_DIRECTORY = \"{}\"".format(output_dir),
            "CREATE_SUBDIRS = NO",
            "ALLOW_UNICODE_NAMES = YES",
            "OPTIMIZE_OUTPUT_FOR_C = NO",
            "OPTIMIZE_OUTPUT_JAVA = NO",
            "OPTIMIZE_FOR_FORTRAN = NO",
            "OPTIMIZE_OUTPUT_VHDL = NO",
            "EXTRACT_ALL = YES",
            "EXTRACT_PRIVATE = NO",
            "EXTRACT_PRIV_VIRTUAL = NO",
            "EXTRACT_PACKAGE = NO",
            "EXTRACT_STATIC = NO",
            "EXTRACT_LOCAL_CLASSES = YES",
            "EXTRACT_LOCAL_METHODS = NO",
            "EXTRACT_ANON_NSPACES = NO",
            "HIDE_UNDOC_MEMBERS = NO",
            "HIDE_UNDOC_CLASSES = NO",
            "HIDE_FRIEND_COMPOUNDS = NO",
            "HIDE_IN_BODY_DOCS = NO",
            "INPUT = \"{}\"".format(src_dir),
            "FILE_PATTERNS = *.h *.hpp *.hh *.hxx *.c *.cc *.cpp *.cxx",
            "RECURSIVE = YES",
            "EXCLUDE =",
            "EXCLUDE_PATTERNS =",
            "GENERATE_HTML = YES",
            "HTML_OUTPUT = doxygen-html",
            "GENERATE_LATEX = NO",
            "GENERATE_XML = NO",
            "QUIET = NO",
            "WARN_IF_UNDOCUMENTED = YES",
            "WARN_AS_ERROR = NO",
        ]
    )


def main() -> int:
    script_path = Path(__file__).resolve()
    repo_root = script_path.parents[2]

    src_dir = repo_root / "src"
    if not src_dir.is_dir():
        print(f"error: source directory not found: {src_dir}", file=sys.stderr)
        return 2

    doxygen_executable = shutil.which("doxygen")
    if doxygen_executable is None:
        print(
            "error: 'doxygen' is not installed or not in PATH. install doxygen and try again.",
            file=sys.stderr,
        )
        return 127

    print(f"using source directory: {src_dir}")
    print(f"writing html output to: {repo_root / 'doc' / 'doxygen-html'}")
    print("note: this script uses a generated config and does not rely on doc\\doxygen\\Doxyfile")

    doxyfile_content = build_doxyfile(repo_root)

    with tempfile.NamedTemporaryFile("w", suffix=".doxyfile", delete=False, encoding="utf-8") as handle:
        handle.write(doxyfile_content)
        temp_doxyfile = Path(handle.name)

    try:
        result = subprocess.run(
            [doxygen_executable, str(temp_doxyfile)],
            cwd=repo_root,
            check=False,
        )
        return result.returncode
    finally:
        temp_doxyfile.unlink(missing_ok=True)


if __name__ == "__main__":
    raise SystemExit(main())
