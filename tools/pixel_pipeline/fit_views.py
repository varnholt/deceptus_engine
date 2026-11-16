"""Finds the camera of each Zero123++ view of the AI model by matching the model's silhouette to the view's.

python fit_views.py <mesh npz> <views directory> <output json>

Zero123++ v1.2 paints six views around its input at azimuths 30, 90, ... 330 and elevations 20 / -10, with a
30 degree field of view, but the painted figure is never exactly where those numbers put it. So each view's
camera (azimuth, elevation, distance and where it looks) is searched until the projected model covers the
view's figure best. The views show the figure on flat gray, which gives the figure's outline.
"""

import argparse
import glob
import json
import math
import os

import numpy
from PIL import Image
from scipy import ndimage, optimize

VIEW_SIZE = 320
FIELD_OF_VIEW = math.radians(30.0)
BACKGROUND = numpy.array([128, 128, 127])
# how far (degrees) a view's azimuth may move from Zero123++'s nominal one
AZIMUTH_REACH = 15.0
NOMINAL = {30: 20.0, 90: -10.0, 150: 20.0, 210: -10.0, 270: 20.0, 330: -10.0}


def view_mask(path):
    pixels = numpy.asarray(Image.open(path).convert("RGB")).astype(int)
    mask = numpy.abs(pixels - BACKGROUND).sum(axis=-1) > 14
    labels, count = ndimage.label(mask)
    if count > 1:
        sizes = ndimage.sum(mask, labels, range(1, count + 1))
        mask = labels == (1 + int(numpy.argmax(sizes)))
    return ndimage.binary_fill_holes(mask)


def camera(parameters, sign):
    """Camera position, its axes (right, up, forward) from azimuth, elevation (degrees), distance and target (x, z)."""
    azimuth, elevation, distance, target_x, target_z = parameters
    azimuth, elevation = math.radians(azimuth) * sign, math.radians(elevation)
    direction = numpy.array([math.cos(elevation) * math.sin(azimuth), -math.cos(elevation) * math.cos(azimuth), math.sin(elevation)])
    target = numpy.array([target_x, 0.0, target_z])
    position = target + distance * direction
    forward = -direction
    right = numpy.cross(forward, [0.0, 0.0, 1.0])
    right /= numpy.linalg.norm(right)
    up = numpy.cross(right, forward)
    return position, right, up, forward


def project(points, parameters, sign):
    """Pixel columns, rows and depths of points seen by the view's camera."""
    position, right, up, forward = camera(parameters, sign)
    relative = points - position
    depth = relative @ forward
    focal = (VIEW_SIZE / 2) / math.tan(FIELD_OF_VIEW / 2)
    column = VIEW_SIZE / 2 + focal * (relative @ right) / depth
    row = VIEW_SIZE / 2 - focal * (relative @ up) / depth
    return column, row, depth


def silhouette(points, parameters, sign):
    column, row, _ = project(points, parameters, sign)
    inside = (column >= 0) & (column < VIEW_SIZE) & (row >= 0) & (row < VIEW_SIZE)
    mask = numpy.zeros((VIEW_SIZE, VIEW_SIZE), bool)
    mask[row[inside].astype(int), column[inside].astype(int)] = True
    mask = ndimage.binary_closing(mask, iterations=2)
    return ndimage.binary_fill_holes(mask)


def overlap(first, second):
    return (first & second).sum() / max((first | second).sum(), 1)


def fit_view(points, mask, azimuth, sign):
    rows = numpy.nonzero(mask.any(axis=1))[0]
    focal = (VIEW_SIZE / 2) / math.tan(FIELD_OF_VIEW / 2)
    height = points[:, 2].max() - points[:, 2].min()
    distance = focal * height / (rows.max() - rows.min() + 1)
    elevation = NOMINAL[azimuth]

    def full(free):
        """Zero123++ renders each view at a fixed elevation: only azimuth, distance and target are searched,
        the azimuth near its nominal value and the target within the figure (side views otherwise slide off into
        wrong fits)."""
        azimuth_value, distance_value, target_x, target_z = free
        azimuth_value = float(numpy.clip(azimuth_value, azimuth - AZIMUTH_REACH, azimuth + AZIMUTH_REACH))
        return [azimuth_value, elevation, distance_value, float(numpy.clip(target_x, -0.3, 0.3)), float(numpy.clip(target_z, -0.4, 0.4))]

    def cost(free):
        return 1.0 - overlap(silhouette(points, full(free), sign), mask)

    best = (cost([azimuth, distance, 0.0, 0.0]), [azimuth, distance, 0.0, 0.0])
    # coarse search over where it looks, then a local refinement of everything
    for target_z in numpy.linspace(-0.3, 0.3, 7):
        for target_x in numpy.linspace(-0.2, 0.2, 5):
            free = [azimuth, distance, target_x, target_z]
            value = cost(free)
            if value < best[0]:
                best = (value, free)
    result = optimize.minimize(cost, best[1], method="Powell", options={"xtol": 1e-3, "ftol": 1e-4, "maxfev": 600})
    result.x = full(result.x)
    return list(map(float, result.x)), 1.0 - float(result.fun)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("mesh")
    parser.add_argument("views")
    parser.add_argument("output")
    arguments = parser.parse_args()
    points = numpy.load(arguments.mesh)["vertices"][::4]
    fits = {}
    for path in sorted(glob.glob(os.path.join(arguments.views, "view_*.png"))):
        azimuth = int(os.path.basename(path)[5:8])
        mask = view_mask(path)
        # silhouettes cannot tell left from right (a profile mirrors its other side); the face in the views can:
        # it shows Zero123++ turns toward the model's +x, so the sign is fixed
        parameters, score = fit_view(points, mask, azimuth, 1)
        fits[os.path.basename(path)] = {"parameters": parameters, "sign": 1, "overlap": score}
        print(os.path.basename(path), [round(value, 3) for value in parameters], round(score, 3), flush=True)
    json.dump(fits, open(arguments.output, "w"), indent=1)
