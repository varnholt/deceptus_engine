#!/usr/bin/env python3
import json
import sys
from pathlib import Path


def format_json(value, indent=0, indent_step=3):
    space = " " * indent

    if isinstance(value, dict):
        if not value:
            return "{}"

        parts = []
        for key, item in value.items():
            formatted_item = format_json(item, indent + indent_step, indent_step)
            parts.append(
                " " * (indent + indent_step)
                + json.dumps(key)
                + ": "
                + formatted_item
            )

        return "{\n" + ",\n".join(parts) + "\n" + space + "}"

    if isinstance(value, list):
        return "[" + ", ".join(json.dumps(item) if not isinstance(item, (dict, list)) else format_json(item, 0, indent_step) for item in value) + "]"

    return json.dumps(value)


def main():
    if len(sys.argv) != 2:
        print("usage: python json_formatter.py <file.json>")
        sys.exit(1)

    input_path = Path(sys.argv[1])
    text = input_path.read_text(encoding="utf-8")

    data = json.loads(text)
    formatted = format_json(data) + "\n"

    input_path.write_text(formatted, encoding="utf-8")
    print(f"formatted {input_path}")


if __name__ == "__main__":
    main()