"""Regenerate data/sprites/eye_positions.json from the player sprite sheet.

Two things this does differently from lab/eye_position_detector:

- it keeps one entry per animation frame. That tool shared an "already seen this sprite sheet
  region" set across animations and skipped repeated regions outright, so any cycle reusing frames
  of an earlier cycle ended up with a short - and positionally misaligned - eye position array, or
  with no entry at all. Every sword cycle drawn on top of an earlier row lost its entry that way.
- it matches the eye near its colour rather than exactly. The sheet was authored in blocks and the
  later ones carry a slightly shifted palette, so the eye reads (51, 20, 35) in the oldest rows,
  (51, 23, 37) in the sword rows and (54, 23, 50) in the double jump sword row. Exact matching found
  nothing at all in those.

The eye is identified structurally, not by colour alone: a dark pixel two pixels tall, sitting next
to skin, with an identically coloured partner two or three pixels to the side - the other eye.
"""

import argparse
import json
from collections import defaultdict
from pathlib import Path

from PIL import Image

REPO_ROOT = Path(__file__).resolve().parents[2]
PLAYER_TEXTURE_PATH = REPO_ROOT / "data" / "sprites" / "player.png"
ANIMATIONS_PATH = REPO_ROOT / "data" / "sprites" / "animations.json"
EYE_POSITIONS_PATH = REPO_ROOT / "data" / "sprites" / "eye_positions.json"

EYE_COLOR = (0x33, 0x14, 0x23)
EYE_COLOR_TOLERANCE = 22

SKIN_COLORS = [(219, 150, 107), (200, 140, 107), (190, 107, 69), (171, 104, 74)]
SKIN_COLOR_TOLERANCE = 12

EYE_SPACING_MIN = 2
EYE_SPACING_MAX = 3

# cycles the player animation builds by reversing another cycle and renaming it
REVERSED_CYCLES = {
    "player_bend_up_l": "player_bend_down_l",
    "player_bend_up_r": "player_bend_down_r",
    "player_bend_up_sword_l": "player_bend_down_sword_l",
    "player_bend_up_sword_r": "player_bend_down_sword_r",
    "player_dash_stop_l": "player_dash_init_l",
    "player_dash_stop_r": "player_dash_init_r",
}


def get_side(animation_name):
    if animation_name.endswith("_l"):
        return "l"
    if animation_name.endswith("_r"):
        return "r"
    return None


def is_near(color, reference, tolerance):
    return (
        abs(color[0] - reference[0]) <= tolerance
        and abs(color[1] - reference[1]) <= tolerance
        and abs(color[2] - reference[2]) <= tolerance
    )


def is_skin(color):
    return color[3] == 255 and any(is_near(color, skin_color, SKIN_COLOR_TOLERANCE) for skin_color in SKIN_COLORS)


def find_eye(pixels, frame_x, frame_y, frame_width, frame_height, side):
    """Locate the eye inside one frame and return frame local coordinates, or None."""
    candidates_by_color = defaultdict(list)

    for local_y in range(frame_height - 1):
        for local_x in range(frame_width):
            color = pixels[frame_x + local_x, frame_y + local_y]
            if color[3] != 255 or not is_near(color, EYE_COLOR, EYE_COLOR_TOLERANCE):
                continue
            if pixels[frame_x + local_x, frame_y + local_y + 1] != color:
                continue

            neighbours = []
            for delta_x in (-1, 1):
                neighbour_x = local_x + delta_x
                if 0 <= neighbour_x < frame_width:
                    neighbours.append(pixels[frame_x + neighbour_x, frame_y + local_y])
                    neighbours.append(pixels[frame_x + neighbour_x, frame_y + local_y + 1])
            if not any(is_skin(neighbour) for neighbour in neighbours):
                continue

            candidates_by_color[color[:3]].append((local_x, local_y))

    # only a candidate with a partner beside it is an eye; every other dark two pixel run next to
    # skin is a nostril, a strand of hair or a fold in the collar
    paired = []
    for candidates in candidates_by_color.values():
        for candidate in candidates:
            has_partner = any(
                other is not candidate
                and EYE_SPACING_MIN <= abs(other[0] - candidate[0]) <= EYE_SPACING_MAX
                and abs(other[1] - candidate[1]) <= 1
                for other in candidates
            )
            if has_partner:
                paired.append(candidate)

    if not paired:
        return None

    top_row_y = min(candidate[1] for candidate in paired)
    top_row = [candidate for candidate in paired if candidate[1] == top_row_y]
    if side == "l":
        return min(top_row, key=lambda candidate: candidate[0])
    return max(top_row, key=lambda candidate: candidate[0])


def fill_gaps(detected):
    """Interpolate frames without a detected eye from the nearest detected neighbours."""
    filled = list(detected)
    known_indices = [index for index, position in enumerate(filled) if position is not None]
    if not known_indices:
        return None

    for index, position in enumerate(filled):
        if position is not None:
            continue

        previous_indices = [known_index for known_index in known_indices if known_index < index]
        next_indices = [known_index for known_index in known_indices if known_index > index]

        if previous_indices and next_indices:
            previous_index = previous_indices[-1]
            next_index = next_indices[0]
            blend = (index - previous_index) / (next_index - previous_index)
            previous_position = filled[previous_index]
            next_position = filled[next_index]
            filled[index] = (
                int(round(previous_position[0] + blend * (next_position[0] - previous_position[0]))),
                int(round(previous_position[1] + blend * (next_position[1] - previous_position[1]))),
            )
        elif previous_indices:
            filled[index] = filled[previous_indices[-1]]
        else:
            filled[index] = filled[next_indices[0]]

    return filled


def detect_cycle(pixels, animation, side):
    frame_offset_x, frame_offset_y = animation["frame_offset"]
    frame_width, frame_height = animation["frame_size"]
    sprite_count = animation["sprite_count"]

    detected = []
    for frame_index in range(sprite_count):
        frame_x = frame_offset_x + frame_index * frame_width
        detected.append(find_eye(pixels, frame_x, frame_offset_y, frame_width, frame_height, side))

    detected_count = sum(1 for position in detected if position is not None)
    return fill_gaps(detected), detected_count


def main():
    parser = argparse.ArgumentParser(description="regenerate eye_positions.json from the player sprite sheet")
    parser.add_argument("--write", action="store_true", help="write the result to data/sprites/eye_positions.json")
    parser.add_argument("--out", default="", help="write the result here instead of the data folder")
    arguments = parser.parse_args()

    player_texture = Image.open(PLAYER_TEXTURE_PATH).convert("RGBA")
    pixels = player_texture.load()

    with open(ANIMATIONS_PATH) as animations_file:
        animations = json.load(animations_file)

    result = {}
    undetected_cycles = []

    for animation_name in sorted(animations):
        if not animation_name.startswith("player_"):
            continue
        side = get_side(animation_name)
        if side is None:
            continue

        positions, detected_count = detect_cycle(pixels, animations[animation_name], side)
        if positions is None:
            undetected_cycles.append(animation_name)
            continue

        result[animation_name] = {
            "eye_positions_x": [position[0] for position in positions],
            "eye_positions_y": [position[1] for position in positions],
        }
        if detected_count < len(positions):
            print(f"{animation_name}: {detected_count}/{len(positions)} frames detected, rest interpolated")

    for reversed_name, source_name in REVERSED_CYCLES.items():
        source = result.get(source_name)
        if source is None:
            continue
        result[reversed_name] = {
            "eye_positions_x": list(reversed(source["eye_positions_x"])),
            "eye_positions_y": list(reversed(source["eye_positions_y"])),
        }

    print(f"\nno eye found in any frame: {', '.join(undetected_cycles) if undetected_cycles else 'none'}")

    output_path = Path(arguments.out) if arguments.out else EYE_POSITIONS_PATH
    if arguments.write or arguments.out:
        write_json(output_path, result)
        print(f"wrote {len(result)} cycles to {output_path}")
    else:
        print(f"{len(result)} cycles detected (dry run, nothing written)")


def write_json(output_path, result):
    """Write the same shape the file already has: three space indent, one line per position array."""
    lines = ["{"]
    names = sorted(result)
    for name_index, name in enumerate(names):
        lines.append(f'   "{name}": {{')
        lines.append(f'      "eye_positions_x": {json.dumps(result[name]["eye_positions_x"])},')
        lines.append(f'      "eye_positions_y": {json.dumps(result[name]["eye_positions_y"])}')
        lines.append("   }" + ("," if name_index + 1 < len(names) else ""))
    lines.append("}")

    with open(output_path, "w", newline="\r\n") as output_file:
        output_file.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
