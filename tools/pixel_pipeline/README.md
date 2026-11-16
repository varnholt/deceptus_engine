# Pixel pipeline

Experimental tools for fitting Blender characters to the original player artwork,
rendering views and preparing palette-constrained sprite frames.

- `palette.json` defines the game color ramps and lighting choices.
- `blender/` contains model preparation, rigging, projection and rendering scripts.
  Run these inside Blender; they require its `bpy` module.
- `head_library.py` extracts heads from the original frames.
- `finish_frames.py`, `downscale.py` and `make_gifs.py` prepare rendered frames.
- `compare_animation.py`, `match_metrics.py` and `fit_views.py` support comparisons
  against the original artwork.

The image-processing scripts use Pillow and NumPy. Read each script's module
instructions or command-line help for its arguments. Some scripts use local
model paths and render directories that must be configured for the working project.
Blender project files, backup versions and rendered output are maintained locally
and are not included with these source scripts.

The source artwork remains governed by the game-content notices in `data/`.
