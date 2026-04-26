"""extract-translations — collect all translatable strings into a locale json file.

mirroring qt's lupdate workflow: source text is the key. running this tool
produces (or updates) data/locale/<locale>.json with every unique source string
found in c++, lua, and tmx files.

existing translations in the output file are preserved. new strings are added
with an empty value so untranslated text falls back to english at runtime.

usage:
    uv run extract-translations                 # updates data/locale/en.json
    uv run extract-translations --locale de     # creates/updates de.json template
    uv run extract-translations --root /other   # run from a different repo root
"""

import argparse
import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path


# matches: tr("some source text")
# the source text may contain escaped quotes via \" but not raw unescaped ones
_TR_PATTERN = re.compile(r'\btr\(\s*"((?:[^"\\]|\\.)*)"\s*\)')


def extract_from_source_files(root: Path) -> list[str]:
    """collect unique tr() source strings from c++ and lua files under root."""
    found: list[str] = []
    seen: set[str] = set()

    for extension in ("*.cpp", "*.h", "*.lua"):
        for source_path in sorted(root.rglob(extension)):
            try:
                text = source_path.read_text(encoding="utf-8", errors="ignore")
            except OSError:
                continue
            for match in _TR_PATTERN.finditer(text):
                raw = match.group(1)
                source_text = raw.replace('\\"', '"')
                if source_text not in seen:
                    seen.add(source_text)
                    found.append(source_text)

    return found


def extract_from_tmx_files(root: Path) -> list[str]:
    """collect unique dialogue text values from all tmx files under root.

    only properties whose name matches a two-digit index (01, 02 ...) inside
    objectgroups named 'dialogues' are extracted, mirroring how Dialogue::deserialize
    reads them.
    """
    found: list[str] = []
    seen: set[str] = set()

    for tmx_path in sorted(root.rglob("*.tmx")):
        try:
            tree = ET.parse(tmx_path)
        except ET.ParseError as parse_error:
            print(f"warning: could not parse {tmx_path}: {parse_error}")
            continue

        for objectgroup in tree.getroot().iter("objectgroup"):
            if objectgroup.get("name", "").lower() != "dialogues":
                continue

            for obj in objectgroup.iter("object"):
                properties_element = obj.find("properties")
                if properties_element is None:
                    continue

                for prop in properties_element.findall("property"):
                    if not re.fullmatch(r"\d{2}", prop.get("name", "")):
                        continue

                    source_text = prop.get("value", "").strip()
                    if source_text and source_text not in seen:
                        seen.add(source_text)
                        found.append(source_text)

    return found


def extract_from_inventory_json(root: Path) -> list[str]:
    """collect title and description strings from inventory_items.json."""
    found: list[str] = []
    seen: set[str] = set()

    inventory_path = root / "data" / "sprites" / "inventory_items.json"
    if not inventory_path.exists():
        return found

    try:
        data = json.loads(inventory_path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError) as load_error:
        print(f"warning: could not load {inventory_path}: {load_error}")
        return found

    for item in data.values():
        for field in ("title", "description"):
            source_text = item.get(field, "").strip()
            if source_text and source_text not in seen:
                seen.add(source_text)
                found.append(source_text)

    return found


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--locale", default="en", help="locale identifier written to the output file (default: en)")
    parser.add_argument("--output", default="data/locale", help="output directory relative to --root (default: data/locale)")
    parser.add_argument("--root", default=".", help="repository root directory (default: current directory)")
    args = parser.parse_args()

    repo_root = Path(args.root).resolve()
    output_dir = repo_root / args.output
    output_dir.mkdir(parents=True, exist_ok=True)
    output_path = output_dir / f"{args.locale}.json"

    existing: dict[str, str] = {}
    if output_path.exists():
        try:
            existing = json.loads(output_path.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError) as load_error:
            print(f"warning: could not load existing {output_path}: {load_error}")

    source_strings = extract_from_source_files(repo_root / "src")
    source_strings += extract_from_source_files(repo_root / "data")
    source_strings += extract_from_tmx_files(repo_root / "data")
    source_strings += extract_from_inventory_json(repo_root)

    # deduplicate while preserving first-seen order
    unique_strings: list[str] = []
    unique_set: set[str] = set()
    for source_string in source_strings:
        if source_string not in unique_set:
            unique_set.add(source_string)
            unique_strings.append(source_string)

    # existing translation takes priority; new keys get empty string (needs translating)
    merged: dict[str, str] = {source: existing.get(source, "") for source in unique_strings}

    output_path.write_text(json.dumps(merged, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    new_keys = [source for source in merged if source not in existing]
    print(f"wrote {len(merged)} strings to {output_path}")
    if new_keys:
        print(f"  {len(new_keys)} new string(s) added:")
        for key in new_keys:
            preview = key[:60] + "..." if len(key) > 60 else key
            print(f"    {preview!r}")
