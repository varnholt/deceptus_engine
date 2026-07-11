#!/usr/bin/env python3
"""
Migrate SFML2 API calls to VRSFML (vittorioromeo's SFML rewrite).

Processes all .cpp and .h files under src/ (excluding src/wasm/).
Applies in-place substitutions for setter→member, getter→member,
Sprite construction, RenderStates, and sf::String.
"""

import os
import re
import sys


def find_matching_close_paren(text, open_index):
    """
    Given text and the index of an opening '(', return the index of the
    matching closing ')'. Returns -1 if not found.
    """
    depth = 0
    for index in range(open_index, len(text)):
        if text[index] == '(':
            depth += 1
        elif text[index] == ')':
            depth -= 1
            if depth == 0:
                return index
    return -1


def replace_method_calls_in_line(line, method_name, replacement_member, wrap_value_prefix="", wrap_value_suffix=""):
    """
    Replace all occurrences of ->method_name(args) or .method_name(args)
    in a single line with ->replacement_member = args or .replacement_member = args.

    If wrap_value_prefix/suffix are provided, the extracted argument is wrapped:
    ->member = wrap_value_prefix + args + wrap_value_suffix
    """
    result = []
    search_start = 0
    text = line

    arrow_pattern = re.compile(r'(->|\.)\s*' + re.escape(method_name) + r'\s*\(')

    while search_start < len(text):
        match = arrow_pattern.search(text, search_start)
        if not match:
            result.append(text[search_start:])
            break

        accessor = match.group(1)   # '->' or '.'
        open_paren_index = match.end() - 1  # index of '('

        close_paren_index = find_matching_close_paren(text, open_paren_index)
        if close_paren_index == -1:
            # No matching close paren found — leave the rest unchanged
            result.append(text[search_start:])
            break

        argument = text[open_paren_index + 1:close_paren_index]

        # Build replacement
        if wrap_value_prefix or wrap_value_suffix:
            new_value = wrap_value_prefix + argument + wrap_value_suffix
        else:
            new_value = argument

        replacement = accessor + replacement_member + ' = ' + new_value

        result.append(text[search_start:match.start()])
        result.append(replacement)
        search_start = close_paren_index + 1

    return ''.join(result)


def apply_simple_regex(line, pattern, replacement):
    """Apply a simple regex substitution to a line."""
    return re.sub(pattern, replacement, line)


def process_line(line):
    """Apply all SFML2→VRSFML substitutions to a single line."""

    # --- Setter → public member ---
    line = replace_method_calls_in_line(line, 'setPosition', 'position')
    line = replace_method_calls_in_line(line, 'setOrigin', 'origin')
    line = replace_method_calls_in_line(line, 'setScale', 'scale')
    line = replace_method_calls_in_line(line, 'setTextureRect', 'textureRect')
    line = replace_method_calls_in_line(line, 'setColor', 'color')
    line = replace_method_calls_in_line(
        line,
        'setRotation',
        'rotation',
        wrap_value_prefix='sf::degrees(',
        wrap_value_suffix=')'
    )

    # --- Getter → public member ---
    line = apply_simple_regex(line, r'->getPosition\(\)', '->position')
    line = apply_simple_regex(line, r'\.getPosition\(\)', '.position')
    line = apply_simple_regex(line, r'->getOrigin\(\)', '->origin')
    line = apply_simple_regex(line, r'\.getOrigin\(\)', '.origin')
    line = apply_simple_regex(line, r'->getScale\(\)', '->scale')
    line = apply_simple_regex(line, r'\.getScale\(\)', '.scale')
    line = apply_simple_regex(line, r'->getTextureRect\(\)', '->textureRect')
    line = apply_simple_regex(line, r'\.getTextureRect\(\)', '.textureRect')
    line = apply_simple_regex(line, r'->getRotation\(\)', '->rotation')
    line = apply_simple_regex(line, r'\.getRotation\(\)', '.rotation')

    # --- Sprite construction: remove texture arg from make_unique / make_shared ---
    # std::make_unique<sf::Sprite>(*expr) or std::make_unique<sf::Sprite>(expr)
    line = apply_simple_regex(
        line,
        r'std::make_unique<sf::Sprite>\(\*[^)]+\)',
        'std::make_unique<sf::Sprite>()'
    )
    line = apply_simple_regex(
        line,
        r'std::make_unique<sf::Sprite>\([^)]+\)',
        'std::make_unique<sf::Sprite>()'
    )
    line = apply_simple_regex(
        line,
        r'std::make_shared<sf::Sprite>\(\*[^)]+\)',
        'std::make_shared<sf::Sprite>()'
    )
    line = apply_simple_regex(
        line,
        r'std::make_shared<sf::Sprite>\([^)]+\)',
        'std::make_shared<sf::Sprite>()'
    )

    # --- RenderStates::Default → RenderStates{} ---
    line = apply_simple_regex(
        line,
        r'sf::RenderStates::Default',
        'sf::RenderStates{}'
    )

    # --- sf::String → std::string ---
    line = apply_simple_regex(line, r'\bsf::String\b', 'std::string')

    return line


def process_file(file_path):
    """Read, transform, and write back a single source file."""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='replace') as source_file:
            original_lines = source_file.readlines()
    except OSError as error:
        print(f'ERROR reading {file_path}: {error}', file=sys.stderr)
        return False

    transformed_lines = [process_line(line) for line in original_lines]

    if transformed_lines == original_lines:
        return False  # No changes

    try:
        with open(file_path, 'w', encoding='utf-8') as destination_file:
            destination_file.writelines(transformed_lines)
    except OSError as error:
        print(f'ERROR writing {file_path}: {error}', file=sys.stderr)
        return False

    return True


def collect_source_files(root_directory, exclude_directory):
    """Yield all .cpp and .h file paths under root_directory, skipping exclude_directory."""
    exclude_real = os.path.realpath(exclude_directory)
    for dirpath, dirnames, filenames in os.walk(root_directory):
        real_dirpath = os.path.realpath(dirpath)
        if real_dirpath.startswith(exclude_real):
            dirnames.clear()
            continue
        for filename in filenames:
            if filename.endswith('.cpp') or filename.endswith('.h'):
                yield os.path.join(dirpath, filename)


def main():
    script_path = os.path.realpath(__file__)
    wasm_directory = os.path.dirname(script_path)
    src_directory = os.path.dirname(wasm_directory)

    print(f'Source root : {src_directory}')
    print(f'Excluding   : {wasm_directory}')

    changed_count = 0
    processed_count = 0

    for source_file_path in sorted(collect_source_files(src_directory, wasm_directory)):
        processed_count += 1
        was_changed = process_file(source_file_path)
        if was_changed:
            changed_count += 1
            print(f'  CHANGED: {source_file_path}')

    print(f'\nDone. Processed {processed_count} files, changed {changed_count}.')


if __name__ == '__main__':
    main()
