"""Colors every vertex of the AI model from the Zero123++ views that see it (cameras from fit_views.py).

python view_colors.py <mesh npz> <views directory> <fits json> <output npy>

Each view paints the side it faces. A vertex takes the colors of the views that see it: in front of the
view's depth (no other part covers it) and facing the camera, blended by how directly they face it. The AI
painted some views slimmer than the mesh, so a vertex that lands just outside the painted figure takes the
nearest figure pixel instead of the gray background; one that lands far outside skips that view. Each image
row is also stretched so the model's edges meet the painted figure's. Vertices
no view sees keep a zero weight (the output's fourth column), so the caller can leave them as they were.
"""

import argparse
import json
import os

import numpy
from PIL import Image
from scipy import ndimage

import fit_views

# how much nearer than the view's depth map a vertex must be to count as seen, in model units
DEPTH_TOLERANCE = 0.03
# pixels a vertex may land outside the painted figure and still take its nearest edge color
EDGE_REACH = 6
# facing below this (cosine between normal and the direction to the camera) is too edge-on to trust
MINIMUM_FACING = 0.2
# a high power lets the most direct view win: blending slightly misaligned views smears the colors
FACING_POWER = 10.0
# how strongly a view's silhouette overlap ranks it against the best fitting view
QUALITY_POWER = 8.0
# the painted figures' edges fade into the gray background; colors are taken this many pixels inside
EDGE_INSET = 3


def splat(column, row):
    size = fit_views.VIEW_SIZE
    inside = (column >= 0) & (column < size) & (row >= 0) & (row < size)
    mask = numpy.zeros((size, size), bool)
    mask[row[inside].astype(int), column[inside].astype(int)] = True
    return ndimage.binary_fill_holes(ndimage.binary_closing(mask, iterations=2))


def row_remap(model_mask, view_mask):
    """Per image row, where the model's left and right edge are and where the painted figure's are, smoothed
    over neighbouring rows. The painting's proportions differ from the mesh (side views are painted slimmer, by
    different amounts at the head, the jacket and the legs), so each row is stretched on its own."""
    size = fit_views.VIEW_SIZE
    edges = numpy.full((size, 4), numpy.nan)
    for row in range(size):
        model_columns = numpy.nonzero(model_mask[row])[0]
        view_columns = numpy.nonzero(view_mask[row])[0]
        if len(model_columns) > 1 and len(view_columns) > 1:
            edges[row] = (model_columns[0], model_columns[-1], view_columns[0], view_columns[-1])
    valid = ~numpy.isnan(edges[:, 0])
    rows = numpy.arange(size)
    for index in range(4):
        edges[:, index] = numpy.interp(rows, rows[valid], edges[valid, index])
        edges[:, index] = ndimage.median_filter(edges[:, index], size=9)
    return edges


def remap_columns(column, row, edges):
    row_index = numpy.clip(row.astype(int), 0, fit_views.VIEW_SIZE - 1)
    model_left, model_right, view_left, view_right = edges[row_index].T
    return view_left + (column - model_left) * (view_right - view_left) / numpy.maximum(model_right - model_left, 1.0)


def depth_map(column, row, depth):
    size = fit_views.VIEW_SIZE
    depths = numpy.full((size, size), numpy.inf)
    inside = (column >= 0) & (column < size) & (row >= 0) & (row < size)
    numpy.minimum.at(depths, (row[inside].astype(int), column[inside].astype(int)), depth[inside])
    # close the gaps between projected vertices
    return ndimage.grey_erosion(numpy.where(numpy.isinf(depths), 1e9, depths), size=3)


def view_colors(vertices, normals, views):
    colors = numpy.zeros((len(vertices), 3))
    weights = numpy.zeros(len(vertices))
    for pixels, mask, parameters, sign, quality in views:
        column, row, depth = fit_views.project(vertices, parameters, sign)
        # rows first (left and right edges), then columns (top and bottom edges) of the row-stretched model
        column = remap_columns(column, row, row_remap(splat(column, row), mask))
        row = remap_columns(row, column, row_remap(splat(row, column), mask.T))
        position, _, _, _ = fit_views.camera(parameters, sign)
        to_camera = position - vertices
        to_camera /= numpy.linalg.norm(to_camera, axis=1, keepdims=True)
        facing = (normals * to_camera).sum(axis=1)
        depths = depth_map(column, row, depth)
        size = fit_views.VIEW_SIZE
        column_index = numpy.clip(column.astype(int), 0, size - 1)
        row_index = numpy.clip(row.astype(int), 0, size - 1)
        seen = (depth <= depths[row_index, column_index] + DEPTH_TOLERANCE) & (facing > MINIMUM_FACING)
        # outside the painted figure: the nearest figure pixel, if it is close; colors only from well inside it
        distance = ndimage.distance_transform_edt(~mask)
        seen &= distance[row_index, column_index] <= EDGE_REACH
        core = ndimage.binary_erosion(mask, iterations=EDGE_INSET)
        _, (nearest_row, nearest_column) = ndimage.distance_transform_edt(~core, return_indices=True)
        sample_row = nearest_row[row_index, column_index]
        sample_column = nearest_column[row_index, column_index]
        weight = numpy.where(seen, numpy.clip(facing, 0.0, 1.0) ** FACING_POWER, 0.0) * quality
        colors += weight[:, None] * pixels[sample_row, sample_column]
        weights += weight
    blended = colors / numpy.maximum(weights, 1e-9)[:, None]
    return numpy.concatenate([blended, weights[:, None]], axis=1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("mesh")
    parser.add_argument("views")
    parser.add_argument("fits")
    parser.add_argument("output")
    arguments = parser.parse_args()
    mesh = numpy.load(arguments.mesh)
    fits = json.load(open(arguments.fits))
    views = []
    best_overlap = max(fit["overlap"] for fit in fits.values())
    for name, fit in fits.items():
        path = os.path.join(arguments.views, name)
        # poorly fitting views only fill in where no better one sees the surface
        quality = (fit["overlap"] / best_overlap) ** QUALITY_POWER
        views.append((numpy.asarray(Image.open(path).convert("RGB")).astype(float), fit_views.view_mask(path), fit["parameters"], fit["sign"], quality))
    result = view_colors(mesh["vertices"], mesh["normals"], views)
    numpy.save(arguments.output, result)
    print(len(result), "vertices,", int((result[:, 3] > 0).sum()), "seen by a view")
