"""Export the splash PSD artwork using data/game/splash/source.json.

Run with: uv run --with psd-tools --with numpy python tools/export_splash.py
Colors are baked into PNGs; playback and layout live in data/config/splash.json.
"""
import json
from pathlib import Path

import numpy as np
from PIL import Image
from psd_tools import PSDImage


def color_bytes(values):
    """Match the renderer's clamped rounding of positive color channels."""
    return np.clip(np.floor(values + np.float32(0.5)), 0, 255).astype(np.uint8)


def export_images(root, recipe):
    """Bake the recipe's layer masks and color grading into runtime textures."""
    psd = PSDImage.open(root / recipe['source'])
    layers = {layer.name: layer for layer in psd.descendants() if not layer.is_group()}
    destination = root / recipe['destination']
    destination.mkdir(exist_ok=True)
    for spec in recipe['images']:
        if 'layer' in spec:
            raster = np.array(layers[spec['layer']].topil().convert('RGBA'))
            height, width = raster.shape[:2]
            if 'gradient' in spec:
                y, x = np.mgrid[:height, :width].astype(np.float32)
                u = (x + np.float32(0.5)) / width * 2 - 1
                v = (y + np.float32(0.5)) / height * 2 - 1
                for channel, coefficients in enumerate(spec['gradient']):
                    a, b, c, d, e, f = map(np.float32, coefficients)
                    raster[:, :, channel] = color_bytes(
                        a + b * u + c * v + d * u * u + e * u * v + f * v * v
                    )
            else:
                raster[:, :, :3] = spec['color']
        else:
            width, height = spec['size']
            y = np.arange(height, dtype=np.float32)
            glow = np.clip(
                (y - spec['gradient_start']) / spec['gradient_length'], 0, 1
            ) ** spec['gradient_power']
            raster = np.full((height, width, 4), 255, dtype=np.uint8)
            raster[:, :, :3] = color_bytes(
                glow[:, None, None] * np.array(spec['color'], dtype=np.float32)
            )
        image = Image.fromarray(raster)
        image.save(destination / spec['file'], optimize=True)
        print(f"{spec['file']}: {image.width} x {image.height}")


def main():
    root = Path(__file__).resolve().parents[1]
    recipe = json.loads((root / 'data/game/splash/source.json').read_text())
    export_images(root, recipe)


if __name__ == '__main__':
    main()
