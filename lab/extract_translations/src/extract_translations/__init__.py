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
import xml.etree.ElementTree as ET
from collections.abc import Iterator
from pathlib import Path


# functions whose call carries a source string, and which of their string literals it is. counting
# literals rather than arguments means the arguments that are not literals do not have to be
# counted; a negative index counts from the end, as python does everywhere else
_SOURCE_TEXT_CALLS = [
    ("tr", 0),  # tr("Quit")
    ("sftr", 0),  # sftr("Quit")
    ("setTitle", 1),  # setTitle("header", "Options", 41), the first literal is a layer name
    ("setCaption", 1),  # setCaption("audio_window-main", "Audio", colour)
    ("updateRowLabel", -1),  # updateRowLabel(_layers["resume_0"], _layers["resume_1"], "Resume")
]

# the source text of a MenuLabel::Label or an InGameMenuLabels::FooterHint, written as a designated
# initializer. MenuLabel puts it through tr() when it draws the label, so it is a source string like
# any other. only read in files that include one of the headers below, since other types have a
# _text member of their own that has nothing to do with translations
_LABEL_TEXT_FIELD = "._text"
_LABEL_HEADERS = ("menulabel.h", "ingamemenulabels.h")

# tables whose entries are translated one at a time rather than written as tr() literals. every
# entry has to be a source string, so a table mixing labels with anything else has to be split
# before it can be listed here
_STRING_TABLES = [
    ("text_speed_strings", "src/menus/menuscreengame.cpp"),
    ("language_display_keys", "src/menus/menuscreengame.cpp"),
    ("tab_names", "src/game/ingamemenu/ingamemenulabels.cpp"),
    ("navigation_labels", "src/game/ingamemenu/ingamemenuarchives.cpp"),
    ("statistics_left_column", "src/game/ingamemenu/ingamemenuarchives.cpp"),
    ("statistics_right_column", "src/game/ingamemenu/ingamemenuarchives.cpp"),
]

# json files whose entries carry source text, and the fields that hold it
_JSON_SOURCES = [
    ("data/sprites/inventory_items.json", ("title", "description")),
    ("data/config/treasures.json", ("name", "description")),
    ("data/config/achievements.json", ("name", "description")),
]


def read_string_literal(text: str, position: int) -> tuple[str, int] | None:
    """reads the string literal that starts at position.

    \\" is the only escape that changes the source text; every other one is left as it was written,
    which is what the translation table holds.

    returns the text between the quotes and the index just past the closing quote, or None when
    position does not point at a quote or the literal is never closed.
    """
    if position >= len(text) or text[position] != '"':
        return None

    characters: list[str] = []
    index = position + 1

    while index < len(text):
        character = text[index]

        if character == "\\" and index + 1 < len(text):
            following = text[index + 1]
            characters.append('"' if following == '"' else character + following)
            index += 2
            continue

        if character == '"':
            return "".join(characters), index + 1

        characters.append(character)
        index += 1

    return None


def _starts_word(text: str, position: int) -> bool:
    """tells whether position is the start of a name rather than the tail of a longer one.

    without this, looking for 'tr(' would also find the 'tr(' inside 'sftr(' and 'substr('.
    """
    if position == 0:
        return True
    previous = text[position - 1]
    return not previous.isalnum() and previous != "_"


def find_call_literals(text: str, function_name: str) -> Iterator[list[str]]:
    """yields the string literals of every call to function_name, in the order they appear in it.

    only literals that are arguments of the call itself count. a literal inside a nested call is an
    argument of that one, which is what tells tr("Quit") apart from tr(entry.at("name").get()).
    """
    opening = function_name + "("
    search_from = 0

    while True:
        call_start = text.find(opening, search_from)
        if call_start < 0:
            return

        search_from = call_start + len(opening)

        if not _starts_word(text, call_start):
            continue

        literals: list[str] = []
        index = search_from
        open_parentheses = 1

        while index < len(text) and open_parentheses > 0:
            character = text[index]

            if character == '"':
                literal = read_string_literal(text, index)
                if literal is None:
                    break
                if open_parentheses == 1:
                    literals.append(literal[0])
                index = literal[1]
                continue

            if character == "(":
                open_parentheses += 1
            elif character == ")":
                open_parentheses -= 1
            elif character == ";":
                # the statement ended before the call did, so this was not a call after all
                break

            index += 1

        yield literals
        search_from = index


def literal_at(literals: list[str], index: int) -> str | None:
    """returns the literal at index, counting from the end when index is negative.

    returns None when the call does not hold that many literals, which is how a call written with a
    variable rather than a literal reports itself.
    """
    if index < 0:
        index += len(literals)
    if 0 <= index < len(literals):
        return literals[index]
    return None


def find_field_literals(text: str, field_name: str) -> Iterator[str]:
    """yields the string literal assigned to a designated initializer such as ._text = "Close"."""
    search_from = 0

    while True:
        field_start = text.find(field_name, search_from)
        if field_start < 0:
            return

        search_from = field_start + len(field_name)

        index = search_from
        while index < len(text) and text[index] in " \t":
            index += 1

        if index >= len(text) or text[index] != "=":
            continue

        index += 1
        while index < len(text) and text[index] in " \t\r\n":
            index += 1

        literal = read_string_literal(text, index)
        if literal is not None:
            yield literal[0]
            search_from = literal[1]


def find_table_literals(text: str, table_name: str) -> list[str] | None:
    """returns the string literals inside the braces that follow table_name.

    returns None when there is no such table, which is worth reporting: the table was renamed or
    moved, and its entries are silently no longer extracted.
    """
    name_start = text.find(table_name)
    if name_start < 0:
        return None

    brace_start = text.find("{", name_start)
    if brace_start < 0:
        return None

    literals: list[str] = []
    index = brace_start + 1
    open_braces = 1

    while index < len(text) and open_braces > 0:
        character = text[index]

        if character == '"':
            literal = read_string_literal(text, index)
            if literal is None:
                break
            literals.append(literal[0])
            index = literal[1]
            continue

        if character == "{":
            open_braces += 1
        elif character == "}":
            open_braces -= 1

        index += 1

    return literals


def is_dialogue_item(property_name: str) -> bool:
    """tells whether a tmx property holds one line of a dialogue, which Dialogue::deserialize reads
    from the two digit names 00, 01, 02 and so on."""
    return len(property_name) == 2 and property_name.isdigit()


def is_interaction_help_text(property_name: str) -> bool:
    """tells whether a tmx property holds the word next to a button icon, which InteractionHelp
    reads from 'text' and from 'text_0', 'text_1' and so on."""
    if property_name == "text":
        return True
    prefix = "text_"
    return property_name.startswith(prefix) and property_name[len(prefix) :].isdigit()


def extract_from_source_files(root: Path) -> list[str]:
    """collect unique source strings from the c++ and lua files under root."""
    found: list[str] = []
    seen: set[str] = set()

    def add(source_text: str) -> None:
        if source_text and source_text not in seen:
            seen.add(source_text)
            found.append(source_text)

    for extension in ("*.cpp", "*.h", "*.lua"):
        for source_path in sorted(root.rglob(extension)):
            try:
                text = source_path.read_text(encoding="utf-8", errors="ignore")
            except OSError:
                continue

            for function_name, literal_index in _SOURCE_TEXT_CALLS:
                for literals in find_call_literals(text, function_name):
                    source_text = literal_at(literals, literal_index)
                    if source_text is not None:
                        add(source_text)

            if any(header in text for header in _LABEL_HEADERS):
                for source_text in find_field_literals(text, _LABEL_TEXT_FIELD):
                    add(source_text)

    return found


def extract_from_string_tables(repo_root: Path) -> list[str]:
    """collect the entries of the tables listed in _STRING_TABLES."""
    found: list[str] = []
    seen: set[str] = set()

    for table_name, relative_path in _STRING_TABLES:
        table_path = repo_root / relative_path
        if not table_path.exists():
            print(f"warning: {relative_path} is gone, {table_name} is no longer extracted")
            continue

        try:
            text = table_path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue

        literals = find_table_literals(text, table_name)
        if literals is None:
            print(f"warning: no table named {table_name} in {relative_path}")
            continue

        for source_text in literals:
            if source_text and source_text not in seen:
                seen.add(source_text)
                found.append(source_text)

    return found


def extract_from_tmx_files(root: Path) -> list[str]:
    """collect unique dialogue and button hint text values from all tmx files under root.

    the object type is not checked, since a property named like either of those is a source string
    wherever it sits.
    """
    found: list[str] = []
    seen: set[str] = set()

    for tmx_path in sorted(root.rglob("*.tmx")):
        try:
            tree = ET.parse(tmx_path)
        except ET.ParseError as parse_error:
            print(f"warning: could not parse {tmx_path}: {parse_error}")
            continue

        for obj in tree.getroot().iter("object"):
            properties_element = obj.find("properties")
            if properties_element is None:
                continue

            for prop in properties_element.findall("property"):
                name = prop.get("name", "")
                if not is_dialogue_item(name) and not is_interaction_help_text(name):
                    continue

                source_text = prop.get("value", "").strip()
                if source_text and source_text not in seen:
                    seen.add(source_text)
                    found.append(source_text)

    return found


def extract_from_json_files(root: Path) -> list[str]:
    """collect the source text of the item, treasure and achievement descriptions."""
    found: list[str] = []
    seen: set[str] = set()

    for relative_path, fields in _JSON_SOURCES:
        json_path = root / relative_path
        if not json_path.exists():
            continue

        try:
            data = json.loads(json_path.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError) as load_error:
            print(f"warning: could not load {json_path}: {load_error}")
            continue

        entries = data.values() if isinstance(data, dict) else data
        for entry in entries:
            if not isinstance(entry, dict):
                continue
            for field in fields:
                value = entry.get(field)
                if not isinstance(value, str):
                    continue
                source_text = value.strip()
                if source_text and source_text not in seen:
                    seen.add(source_text)
                    found.append(source_text)

    return found


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--locale", default="en", help="locale identifier written to the output file (default: en)")
    parser.add_argument("--output", default="data/locale", help="output directory relative to --root (default: data/locale)")
    parser.add_argument("--root", default=".", help="repository root directory (default: current directory)")
    parser.add_argument(
        "--prune",
        action="store_true",
        help="drop keys the extractor no longer finds; without it they are kept and only reported",
    )
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
    source_strings += extract_from_string_tables(repo_root)
    source_strings += extract_from_tmx_files(repo_root / "data")
    source_strings += extract_from_json_files(repo_root)

    # deduplicate while preserving first-seen order. Localization::translate() drops trailing spaces
    # before it looks a string up, so a key that keeps them would never be found at runtime
    unique_strings: list[str] = []
    unique_set: set[str] = set()
    for source_string in source_strings:
        key = source_string.rstrip(" ")
        if key and key not in unique_set:
            unique_set.add(key)
            unique_strings.append(key)

    # a locale file also carries sections that are not translations, such as the line break rules of
    # its script. those are objects rather than strings, and dropping them would take the japanese
    # line breaking with them
    sections = {key: value for key, value in existing.items() if not isinstance(value, str)}
    translations = {key: value for key, value in existing.items() if isinstance(value, str)}

    # existing translation takes priority; new keys get empty string (needs translating)
    merged: dict[str, object] = dict(sections)
    merged.update({source: translations.get(source, "") for source in unique_strings})

    # a key the extractor no longer finds is usually a line that was reworded, but it is also what a
    # gap in the extractor looks like. keeping it costs nothing, and losing a translation to a gap
    # costs a translator's afternoon
    stale_keys = [key for key in translations if key not in unique_set]
    if not args.prune:
        for key in stale_keys:
            merged[key] = translations[key]

    output_path.write_text(json.dumps(merged, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    new_keys = [source for source in unique_strings if source not in translations]
    print(f"wrote {len(merged)} strings to {output_path}")
    if new_keys:
        print(f"  {len(new_keys)} new string(s) added:")
        for key in new_keys:
            preview = key[:60] + "..." if len(key) > 60 else key
            print(f"    {preview!r}")
    if stale_keys:
        verb = "dropped" if args.prune else "no longer found, kept"
        print(f"  {len(stale_keys)} string(s) {verb}:")
        for key in stale_keys:
            preview = key[:60] + "..." if len(key) > 60 else key
            print(f"    {preview!r}")
