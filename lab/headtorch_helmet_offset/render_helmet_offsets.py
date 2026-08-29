"""Render the head torch helmet where the engine places it, for every player animation cycle.

The engine places the helmet at

    helmet_top_left = player_sprite_position_px + eye_position_px + helmet_offset_px

with helmet_offset_px hard coded to (-50, -54) facing right and (-45, -54) facing left, while the
animation frame itself is drawn at

    frame_top_left = player_sprite_position_px + (0, 8) - animation_origin

so this script reproduces both and composites the result, one contact sheet per animation cycle.
"""

import argparse
import json
from pathlib import Path

from PIL import Image, ImageDraw

REPO_ROOT = Path(__file__).resolve().parents[2]
PLAYER_TEXTURE_PATH = REPO_ROOT / "data" / "sprites" / "player.png"
ANIMATIONS_PATH = REPO_ROOT / "data" / "sprites" / "animations.json"
EYE_POSITIONS_PATH = REPO_ROOT / "data" / "sprites" / "eye_positions.json"

HELMET_RECT_RIGHT = (0, 1776, 24, 24)
HELMET_RECT_LEFT = (24, 1776, 24, 24)

BODY_POSITION_OFFSET_PX = (0.0, 8.0)
HELMET_OFFSET_RIGHT_PX = (-50.0, -54.0)
HELMET_OFFSET_LEFT_PX = (-45.0, -54.0)

SCALE = 4
PADDING_PX = 16


def load_inputs(eye_positions_path):
    player_texture = Image.open(PLAYER_TEXTURE_PATH).convert("RGBA")
    with open(ANIMATIONS_PATH) as animations_file:
        animations = json.load(animations_file)
    with open(eye_positions_path) as eye_positions_file:
        eye_positions = json.load(eye_positions_file)
    return player_texture, animations, eye_positions


def get_side(animation_name):
    if animation_name.endswith("_l"):
        return "l"
    if animation_name.endswith("_r"):
        return "r"
    return None


def render_cycle(player_texture, animation, eye_position_list, side, generalized_origin):
    frame_offset_x, frame_offset_y = animation["frame_offset"]
    frame_width, frame_height = animation["frame_size"]
    origin_x, origin_y = animation["origin"]
    sprite_count = animation["sprite_count"]

    helmet_rect = HELMET_RECT_RIGHT if side == "r" else HELMET_RECT_LEFT
    helmet_sprite = player_texture.crop(
        (helmet_rect[0], helmet_rect[1], helmet_rect[0] + helmet_rect[2], helmet_rect[1] + helmet_rect[3])
    )
    helmet_offset_px = HELMET_OFFSET_RIGHT_PX if side == "r" else HELMET_OFFSET_LEFT_PX

    # the sheet cell has to be large enough for the frame plus whatever the helmet sticks out
    cell_width = frame_width + 2 * PADDING_PX
    cell_height = frame_height + 2 * PADDING_PX
    sheet = Image.new("RGBA", (cell_width * sprite_count, cell_height), (24, 24, 32, 255))
    sheet_draw = ImageDraw.Draw(sheet)

    for frame_index in range(sprite_count):
        source_x = frame_offset_x + frame_index * frame_width
        source_y = frame_offset_y
        frame_image = player_texture.crop((source_x, source_y, source_x + frame_width, source_y + frame_height))

        cell_x = frame_index * cell_width
        frame_paste_x = cell_x + PADDING_PX
        frame_paste_y = PADDING_PX
        sheet.alpha_composite(frame_image, (frame_paste_x, frame_paste_y))
        sheet_draw.rectangle(
            [(frame_paste_x, frame_paste_y), (frame_paste_x + frame_width - 1, frame_paste_y + frame_height - 1)],
            outline=(60, 60, 80, 255),
        )

        if eye_position_list is None:
            sheet_draw.line([(frame_paste_x, frame_paste_y), (frame_paste_x + frame_width - 1, frame_paste_y + frame_height - 1)], fill=(255, 0, 0, 255))
            continue

        clamped_index = min(frame_index, len(eye_position_list) - 1)
        eye_position_x, eye_position_y = eye_position_list[clamped_index]

        if generalized_origin:
            # frame_top_left = sprite_position + (0, 8) - origin, and the helmet offset above already
            # folds in the default origin of (36, 48), so undo that and use the real one
            helmet_x = eye_position_x + helmet_offset_px[0] + (36.0 - origin_x)
            helmet_y = eye_position_y + helmet_offset_px[1] + (48.0 - origin_y)
        else:
            helmet_x = eye_position_x + helmet_offset_px[0]
            helmet_y = eye_position_y + helmet_offset_px[1]

        # both are expressed relative to player_sprite_position_px, so convert to frame local coords
        helmet_local_x = helmet_x - (BODY_POSITION_OFFSET_PX[0] - origin_x)
        helmet_local_y = helmet_y - (BODY_POSITION_OFFSET_PX[1] - origin_y)

        sheet.alpha_composite(helmet_sprite, (int(frame_paste_x + helmet_local_x), int(frame_paste_y + helmet_local_y)))
        sheet_draw.rectangle(
            [
                (frame_paste_x + helmet_local_x, frame_paste_y + helmet_local_y),
                (frame_paste_x + helmet_local_x + 23, frame_paste_y + helmet_local_y + 23),
            ],
            outline=(255, 200, 0, 255),
        )
        sheet_draw.line(
            [
                (frame_paste_x + eye_position_x - 2, frame_paste_y + eye_position_y),
                (frame_paste_x + eye_position_x + 2, frame_paste_y + eye_position_y),
            ],
            fill=(0, 255, 255, 255),
        )

    return sheet.resize((sheet.width * SCALE, sheet.height * SCALE), Image.NEAREST)


def main():
    parser = argparse.ArgumentParser(description="render the head torch helmet position per animation cycle")
    parser.add_argument("--out", default="out", help="output directory for the contact sheets")
    parser.add_argument("--filter", default="", help="only render cycles whose name contains this substring")
    parser.add_argument("--generalized-origin", action="store_true", help="apply the animation origin correction")
    parser.add_argument("--eye-positions", default=str(EYE_POSITIONS_PATH), help="eye position json to read")
    arguments = parser.parse_args()

    player_texture, animations, eye_positions = load_inputs(arguments.eye_positions)

    output_directory = Path(arguments.out)
    output_directory.mkdir(parents=True, exist_ok=True)

    for animation_name in sorted(animations):
        if not animation_name.startswith("player_"):
            continue
        side = get_side(animation_name)
        if side is None:
            continue
        if arguments.filter and arguments.filter not in animation_name:
            continue

        eye_entry = eye_positions.get(animation_name)
        if eye_entry is None:
            eye_position_list = None
        else:
            eye_position_list = list(zip(eye_entry["eye_positions_x"], eye_entry["eye_positions_y"]))

        sheet = render_cycle(player_texture, animations[animation_name], eye_position_list, side, arguments.generalized_origin)
        sheet.save(output_directory / f"{animation_name}.png")

    print(f"wrote sheets to {output_directory.resolve()}")


if __name__ == "__main__":
    main()
