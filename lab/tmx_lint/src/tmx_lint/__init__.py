"""tmx-lint — report object names that are reused inside a single tmx map.

the game engine uses an object's name as its identifier, so two objects that
share a name inside the same map are ambiguous and will silently target the
wrong thing at runtime. this tool walks every objectgroup in a map, groups the
named objects by name, and reports any name that appears more than once,
including each occurrence's id, layer and position so the offending object can
be located in tiled.

unnamed objects (for example moving_platform nodes) are ignored on purpose:
they are not used as identifiers, so sharing an empty name is not a conflict.

usage:
    uv run tmx-lint                       # scan every *.tmx under the repo's data/ dir
    uv run tmx-lint data/level-catacombs/catacombs.tmx   # check specific map file(s)
    uv run tmx-lint --root /other/repo    # scan a different repo root
"""

import argparse
import sys
import xml.etree.ElementTree as ElementTree
from collections import defaultdict
from pathlib import Path


class Occurrence:
    """a single named object found in a map, used for reporting a conflict."""

    def __init__(self, object_id, layer_name, position_x, position_y):
        self.object_id = object_id
        self.layer_name = layer_name
        self.position_x = position_x
        self.position_y = position_y


def find_duplicate_names(map_path: Path) -> dict:
    """return a mapping of name -> list[Occurrence] for names used more than once."""
    tree = ElementTree.parse(map_path)

    occurrences_by_name = defaultdict(list)
    for object_group in tree.iter("objectgroup"):
        layer_name = object_group.get("name", "<unnamed layer>")
        for map_object in object_group.iter("object"):
            object_name = map_object.get("name")
            if not object_name:
                continue
            occurrences_by_name[object_name].append(
                Occurrence(
                    object_id=map_object.get("id", "?"),
                    layer_name=layer_name,
                    position_x=map_object.get("x", "?"),
                    position_y=map_object.get("y", "?"),
                )
            )

    return {
        object_name: occurrences
        for object_name, occurrences in occurrences_by_name.items()
        if len(occurrences) > 1
    }


def report_duplicates(map_path: Path, duplicates: dict) -> None:
    """print a human readable report for one map's duplicate names."""
    print(f"{map_path}")
    print(f"  FOUND {len(duplicates)} duplicated name(s):\n")
    for object_name, occurrences in sorted(duplicates.items()):
        print(f'    "{object_name}" used {len(occurrences)} times:')
        for occurrence in occurrences:
            print(
                f"      - id {occurrence.object_id:>5}  "
                f'layer "{occurrence.layer_name}"  '
                f"@ ({occurrence.position_x}, {occurrence.position_y})"
            )
        print()


def collect_map_files(root: Path, explicit_paths: list) -> list:
    """resolve the list of tmx files to check.

    if explicit paths are given they are used verbatim, otherwise every *.tmx
    file under root/data is scanned.
    """
    if explicit_paths:
        return [Path(path) for path in explicit_paths]
    return sorted((root / "data").rglob("*.tmx"))


def main() -> int:
    parser = argparse.ArgumentParser(
        description="report object names reused inside a single tmx map"
    )
    parser.add_argument(
        "maps",
        nargs="*",
        help="tmx file(s) to check; if omitted, every *.tmx under data/ is scanned",
    )
    parser.add_argument(
        "--root",
        default=Path(__file__).resolve().parents[4],
        type=Path,
        help="repo root used when no explicit map files are given",
    )
    arguments = parser.parse_args()

    map_files = collect_map_files(arguments.root, arguments.maps)
    if not map_files:
        print("no tmx files found", file=sys.stderr)
        return 2

    maps_with_duplicates = 0
    for map_path in map_files:
        if not map_path.exists():
            print(f"{map_path}: not found", file=sys.stderr)
            maps_with_duplicates += 1
            continue

        duplicates = find_duplicate_names(map_path)
        if duplicates:
            report_duplicates(map_path, duplicates)
            maps_with_duplicates += 1

    if maps_with_duplicates:
        return 1

    print(f"OK - no duplicate object names in {len(map_files)} map(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
