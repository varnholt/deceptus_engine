"""
@file detect_eye_positions.py
@brief Eye position detection and interpolation for sprite animations
@details Detects player eye positions in sprite sheets, interpolates missing frames,
         and generates visual debug output with color-coded overlays.
"""

import json
import argparse
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Set
from PIL import Image, ImageDraw
from loguru import logger

IMAGE_PATH = Path("player.png")
ANIM_JSON_PATH = Path("animations.json")
EYE_COLOR = (0x33, 0x14, 0x23, 0xff)
MARKER_COLOR = (255, 0, 0)

GREEN_LIGHT = (0, 255, 0, 50)
GREEN_DARK = (0, 180, 0, 50)
RED_LIGHT = (255, 0, 0, 50)
RED_DARK = (180, 0, 0, 50)

FrameData = Tuple[int, int, int, int, int, Optional[Tuple[int, int]], bool]
EyePosition = Tuple[int, int]


def find_eye(pixels, sx: int, sy: int, sw: int, sh: int, side: str) -> Optional[EyePosition]:
    """
    @brief Locate eye pixels within a sprite frame
    @param pixels Pixel access object from PIL image
    @param sx Sprite x origin
    @param sy Sprite y origin
    @param sw Sprite width
    @param sh Sprite height
    @param side Either 'l' (left eye, leftmost) or 'r' (right eye, rightmost)
    @return Absolute (x, y) pixel coordinates of the eye, or None if not found
    """
    candidates = []
    for ly in range(sh - 1):
        for lx in range(sw):
            gx, gy = sx + lx, sy + ly
            if pixels[gx, gy] == EYE_COLOR and pixels[gx, gy + 1] == EYE_COLOR:
                candidates.append((gx, gy))

    if not candidates:
        return None

    if side == "l":
        return candidates[0]
    else:
        top_y = candidates[0][1]
        top_row = [c for c in candidates if c[1] == top_y]
        return max(top_row, key=lambda p: p[0])


def draw_marker(draw: ImageDraw.ImageDraw, gx: int, gy: int) -> None:
    """
    @brief Draw a crosshair marker at the specified position
    @param draw ImageDraw object
    @param gx X coordinate
    @param gy Y coordinate
    """
    r = 3
    draw.line([(gx - r, gy), (gx + r, gy)], fill=MARKER_COLOR)
    draw.line([(gx, gy - r), (gx, gy + r)], fill=MARKER_COLOR)
    draw.line([(gx - r, gy + 1), (gx + r, gy + 1)], fill=MARKER_COLOR)


def detect_eyes_in_all_animations(
    animations: Dict, 
    pixels, 
    seen_regions: Set
) -> Tuple[Dict, Dict]:
    """
    @brief First pass: detect eyes across all animations
    @param animations Animation data dictionary
    @param pixels Pixel access object
    @param seen_regions Set to track processed regions
    @return Tuple of (animation_data_dict, global_eye_positions_dict)
    """
    all_anim_data = {}
    all_frames = {}

    for anim_name, anim in animations.items():
        side = get_animation_side(anim_name)
        if not side:
            continue

        fx_off, fy_off = anim["frame_offset"]
        fw, fh = anim["frame_size"]
        count = anim["sprite_count"]

        frame_data = []
        for i in range(count):
            sx = fx_off + i * fw
            sy = fy_off
            region_key = (sx, sy, fw, fh)
            
            if region_key in seen_regions:
                continue
            seen_regions.add(region_key)

            eye = find_eye(pixels, sx, sy, fw, fh, side)
            frame_data.append((i, sx, sy, fw, fh, eye, eye is not None))
            
            if eye:
                all_frames[(sx, sy, fw, fh)] = eye
        
        all_anim_data[anim_name] = (anim, frame_data)
    
    logger.info(f"detected eyes in {len(all_frames)} unique frames")
    return all_anim_data, all_frames


def get_animation_side(anim_name: str) -> Optional[str]:
    """
    @brief Determine which eye to search for based on animation name
    @param anim_name Animation name
    @return 'l' for left eye, 'r' for right eye, None if not applicable
    """
    if anim_name.endswith("_l"):
        return "l"
    elif anim_name.endswith("_r"):
        return "r"
    return None

def interpolate_missing_eyes(frame_data: List[FrameData], idx: int, all_frames: Dict) -> Optional[EyePosition]:
    """
    @brief Fill in missing eye position using interpolation or fallback strategies
    @param frame_data List of frame data tuples
    @param idx Current frame index
    @param all_frames Global map of detected eye positions
    @return Interpolated eye position or None
    """
    i, sx, sy, fw, fh, eye, found = frame_data[idx]
    
    prev_eye, prev_frame = find_prev_eye(frame_data, idx)
    next_eye, next_frame = find_next_eye(frame_data, idx)
    
    if prev_eye and next_eye:
        return interpolate_between_frames(frame_data, idx, prev_eye, next_eye)
    elif prev_eye:
        return apply_relative_position(sx, sy, prev_eye, prev_frame[1], prev_frame[2])
    elif next_eye:
        return apply_relative_position(sx, sy, next_eye, next_frame[1], next_frame[2])
    else:
        return spatial_fallback(sx, sy, fw, fh, all_frames)


def find_prev_eye(frame_data: List[FrameData], idx: int) -> Tuple[Optional[EyePosition], Optional[FrameData]]:
    """@brief Find the previous frame with a detected eye"""
    for j in range(idx - 1, -1, -1):
        if frame_data[j][6]:
            return frame_data[j][5], frame_data[j]
    return None, None


def find_next_eye(frame_data: List[FrameData], idx: int) -> Tuple[Optional[EyePosition], Optional[FrameData]]:
    """@brief Find the next frame with a detected eye"""
    for j in range(idx + 1, len(frame_data)):
        if frame_data[j][6]:
            return frame_data[j][5], frame_data[j]
    return None, None


def interpolate_between_frames(frame_data: List[FrameData], idx: int, prev_eye: EyePosition, next_eye: EyePosition) -> EyePosition:
    """@brief Linear interpolation between two eye positions"""
    prev_idx = next(j for j in range(idx - 1, -1, -1) if frame_data[j][6])
    next_idx = next(j for j in range(idx + 1, len(frame_data)) if frame_data[j][6])
    t = (idx - prev_idx) / (next_idx - prev_idx)
    return (
        int(prev_eye[0] + t * (next_eye[0] - prev_eye[0])),
        int(prev_eye[1] + t * (next_eye[1] - prev_eye[1]))
    )


def apply_relative_position(sx: int, sy: int, ref_eye: EyePosition, ref_sx: int, ref_sy: int) -> EyePosition:
    """@brief Apply relative eye offset from reference frame to current frame"""
    rel_x = ref_eye[0] - ref_sx
    rel_y = ref_eye[1] - ref_sy
    return (sx + rel_x, sy + rel_y)


def spatial_fallback(sx: int, sy: int, fw: int, fh: int, all_frames: Dict) -> Optional[EyePosition]:
    """@brief Use eye position from left neighbor frame in sprite sheet"""
    left_neighbor_key = (sx - fw, sy, fw, fh)
    if left_neighbor_key in all_frames:
        left_eye = all_frames[left_neighbor_key]
        return apply_relative_position(sx, sy, left_eye, sx - fw, sy)
    return None


def process_interpolation_and_overlays(
    all_anim_data: Dict,
    all_frames: Dict,
    overlay_draw: Optional[ImageDraw.ImageDraw]
) -> Dict:
    """
    @brief Second pass: interpolate missing eyes and draw debug overlays
    @param all_anim_data Animation data from first pass
    @param all_frames Global eye position map
    @param overlay_draw Optional overlay drawing context
    @return Results dictionary with eye positions per animation
    """
    results = {}
    frame_counter = 0

    for anim_name, (anim, frame_data) in all_anim_data.items():
        fw, fh = anim["frame_size"]
        
        for idx in range(len(frame_data)):
            i, sx, sy, fw, fh, eye, found = frame_data[idx]
            
            if not found:
                eye = interpolate_missing_eyes(frame_data, idx, all_frames)
                frame_data[idx] = (i, sx, sy, fw, fh, eye, found)
                if eye:
                    all_frames[(sx, sy, fw, fh)] = eye
            
            if overlay_draw:
                color = (GREEN_LIGHT if found else RED_LIGHT) if frame_counter % 2 == 0 else (GREEN_DARK if found else RED_DARK)
                overlay_draw.rectangle([(sx, sy), (sx + fw - 1, sy + fh - 1)], fill=color)
            
            frame_counter += 1

        anim_results = []
        for i, sx, sy, fw, fh, eye, found in frame_data:
            if eye:
                anim_results.append({
                    "frame": i,
                    "sprite_origin": (sx, sy),
                    "eye_relative": (eye[0] - sx, eye[1] - sy),
                    "interpolated": not found
                })

        if anim_results:
            results[anim_name] = anim_results
    
    return results

def composite_overlays_and_markers(
    marked_img: Image.Image,
    overlay: Image.Image,
    results: Dict
) -> Image.Image:
    """
    @brief Composite debug overlays and draw eye markers
    @param marked_img Base image copy
    @param overlay Transparent overlay with colored rectangles
    @param results Eye detection results
    @return Final composited image
    """
    marked_img = Image.alpha_composite(marked_img.convert('RGBA'), overlay)
    draw = ImageDraw.Draw(marked_img)
    
    for anim_name, frames in results.items():
        for entry in frames:
            sx, sy = entry["sprite_origin"]
            rx, ry = entry["eye_relative"]
            draw_marker(draw, sx + rx, sy + ry)
    
    return marked_img


def print_results(results: Dict) -> None:
    """
    @brief Print eye detection results to console
    @param results Eye detection results dictionary
    """
    total = 0
    total_interpolated = 0
    
    for anim_name, frames in results.items():
        side_label = "left eye" if anim_name.endswith("_l") else "right eye"
        logger.info(f"\n=== {anim_name}  [{side_label}] ===")
        
        for entry in frames:
            sx, sy = entry["sprite_origin"]
            rx, ry = entry["eye_relative"]
            interp_marker = " [INTERPOLATED]" if entry.get("interpolated", False) else ""
            logger.info(f"  frame {entry['frame']}  |  sprite @ ({sx}, {sy})  |  eye (relative) @ ({rx}, {ry}){interp_marker}")
            total += 1
            if entry.get("interpolated", False):
                total_interpolated += 1
    
    logger.success(f"\ntotal unique sprites with eye: {total} (detected: {total - total_interpolated}, interpolated: {total_interpolated})")


def write_animations_with_eyes(animations: Dict, results: Dict, output_path: Path) -> None:
    """
    @brief Write animations.json with eye position data added
    @param animations Original animation data
    @param results Detected eye positions per animation
    @param output_path Output file path
    """
    output_data = {}
    
    for anim_name, anim in animations.items():
        output_data[anim_name] = anim.copy()
        
        if anim_name in results:
            eye_positions_x = []
            eye_positions_y = []
            
            for entry in results[anim_name]:
                rx, ry = entry["eye_relative"]
                eye_positions_x.append(rx)
                eye_positions_y.append(ry)
            
            output_data[anim_name]["eye_positions_x"] = eye_positions_x
            output_data[anim_name]["eye_positions_y"] = eye_positions_y
    
    with open(output_path, 'w') as f:
        json.dump(output_data, f, indent=3)
    
    logger.success(f"animations with eye positions saved: {output_path}")


def main():
    """@brief Main entry point"""
    parser = argparse.ArgumentParser(description="detect player eye positions from animations.json")
    parser.add_argument(
        "--mark",
        metavar="OUTPUT.png",
        help="generate debug image with colored overlays and eye markers"
    )
    parser.add_argument(
        "--output",
        metavar="OUTPUT.json",
        help="write animations.json with eye_positions data added"
    )
    args = parser.parse_args()

    logger.info(f"loading sprite sheet: {IMAGE_PATH}")
    img = Image.open(IMAGE_PATH)
    pixels = img.load()

    logger.info(f"loading animations: {ANIM_JSON_PATH}")
    with open(ANIM_JSON_PATH) as f:
        animations = json.load(f)

    marked_img = img.copy() if args.mark else None
    overlay = Image.new('RGBA', img.size, (0, 0, 0, 0)) if args.mark else None
    overlay_draw = ImageDraw.Draw(overlay) if args.mark else None

    seen_regions = set()
    
    logger.info("pass 1: detecting eyes in all animations")
    all_anim_data, all_frames = detect_eyes_in_all_animations(animations, pixels, seen_regions)
    
    logger.info("pass 2: interpolating missing eyes and generating overlays")
    results = process_interpolation_and_overlays(all_anim_data, all_frames, overlay_draw)
    
    print_results(results)

    if args.mark:
        logger.info("compositing debug image")
        marked_img = composite_overlays_and_markers(marked_img, overlay, results)
        marked_img.save(args.mark)
        logger.success(f"debug image saved: {args.mark}")
    
    if args.output:
        write_animations_with_eyes(animations, results, Path(args.output))


if __name__ == "__main__":
    main()