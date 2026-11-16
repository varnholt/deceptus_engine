"""Cuts dstar's drawn heads out of his frames, so finished frames can use his head drawings.

head_from_frame(frame): the head of one of his frames (hair, face and their outline above the collar)
as an RGBA image plus its top left corner in the frame.

The head is the connected area of hair and skin colors that contains the topmost figure pixel,
limited to the rows above the collar, plus the black outline pixels touching it.
"""

import numpy

HEAD_COLORS = [(140, 40, 92), (155, 44, 105), (188, 54, 116), (214, 67, 118), (102, 30, 80), (171, 50, 109), (212, 66, 117), (194, 59, 115),
               (190, 107, 69), (219, 150, 107), (51, 20, 35), (64, 26, 71),
               # dark hair shades only his run head uses
               (83, 38, 82), (95, 34, 78)]
COLLAR_COLORS = [(119, 49, 81), (208, 67, 73), (13, 63, 83)]


def head_from_frame(frame):
    opaque = frame[..., 3] > 0
    is_head_color = numpy.zeros(opaque.shape, dtype=bool)
    for color in HEAD_COLORS:
        is_head_color |= (frame[..., :3] == color).all(axis=-1) & opaque
    is_collar = numpy.zeros(opaque.shape, dtype=bool)
    for color in COLLAR_COLORS:
        is_collar |= (frame[..., :3] == color).all(axis=-1) & opaque
    collar_rows = numpy.where(is_collar.any(axis=1))[0]
    # the chin sits on the collar: a few rows of the face reach below the collar's first row
    limit = (collar_rows.min() + 3) if len(collar_rows) else frame.shape[0]
    seed_rows, seed_columns = numpy.where(is_head_color)
    start = numpy.argmin(seed_rows)
    region = numpy.zeros_like(is_head_color)
    stack = [(seed_rows[start], seed_columns[start])]
    while stack:
        y, x = stack.pop()
        if region[y, x] or not is_head_color[y, x] or y >= limit:
            continue
        region[y, x] = True
        stack.extend(((y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)))
    black = (frame[..., :3] == 0).all(axis=-1) & opaque
    padded = numpy.pad(region, 1)
    touching = padded[:-2, 1:-1] | padded[2:, 1:-1] | padded[1:-1, :-2] | padded[1:-1, 2:]
    outline = black & touching
    mask = region | outline
    rows, columns = numpy.where(mask)
    top, left = rows.min(), columns.min()
    head = numpy.zeros((rows.max() - top + 1, columns.max() - left + 1, 4), dtype=frame.dtype)
    cropped_mask = mask[top:rows.max() + 1, left:columns.max() + 1]
    head[cropped_mask] = frame[top:rows.max() + 1, left:columns.max() + 1][cropped_mask]
    return head, (int(left), int(top))
