"""check every file a tmx map references actually exists on disk.

resolution rules, from the engine source:
  script            -> data/scripts/enemies/<value>   (level.cpp:667)
  sample / filename -> data/sounds/<value>            (audiobackenddesktop.cpp:17)
  texture           -> repo-relative path as written
  flowfield_texture -> repo-relative path as written
  tileset source    -> relative to the tmx directory
  imagelayer image  -> relative to the tmx directory
"""

import sys
import xml.etree.ElementTree as ElementTree
from collections import defaultdict
from pathlib import Path

tmx_path = Path(sys.argv[1])
repo_root = Path(sys.argv[2]) if len(sys.argv) > 2 else Path(".")
tmx_dir = tmx_path.parent

PREFIXED = {
    "script": "data/scripts/enemies/",
    "sample": "data/sounds/",
    "sample_open": "data/sounds/",
    "sample_locked": "data/sounds/",
    "filename": "data/sounds/",
}
REPO_RELATIVE = {"texture", "flowfield_texture", "normal_map", "texture_normal"}

# some properties resolve under a layer-specific directory rather than as a
# repo-relative path, so the same property name means different things per layer
LAYER_PREFIXED = {
    ("static_lights", "texture"): "data/light/",  # staticlight.cpp:153
    ("lights", "texture"): "data/light/",
}

tree = ElementTree.parse(tmx_path)
references = []  # (kind, raw_value, resolved_path, where)

for group in tree.iter("objectgroup"):
    layer = group.get("name")
    for map_object in group.iter("object"):
        where = f'layer "{layer}" id {map_object.get("id")} name "{map_object.get("name") or ""}"'
        for prop in map_object.iter("property"):
            name, value = prop.get("name"), prop.get("value") or ""
            if not value:
                continue
            if (layer, name) in LAYER_PREFIXED:
                references.append((name, value, repo_root / (LAYER_PREFIXED[(layer, name)] + value), where))
            elif name in PREFIXED:
                references.append((name, value, repo_root / (PREFIXED[name] + value), where))
            elif name in REPO_RELATIVE:
                references.append((name, value, repo_root / value, where))

for tileset in tree.iter("tileset"):
    if tileset.get("source"):
        references.append(("tileset", tileset.get("source"), tmx_dir / tileset.get("source"), "tileset"))

for image_layer in tree.iter("imagelayer"):
    image = image_layer.find("image")
    if image is not None and image.get("source"):
        references.append(
            ("imagelayer", image.get("source"), tmx_dir / image.get("source"), f'imagelayer "{image_layer.get("name")}"')
        )

# tilesets referenced from inside .tsx files resolve relative to the tsx itself
for tileset in tree.iter("tileset"):
    source = tileset.get("source")
    if not source:
        continue
    tsx = tmx_dir / source
    if not tsx.exists():
        continue
    for image in ElementTree.parse(tsx).iter("image"):
        if image.get("source"):
            references.append(("tsx image", image.get("source"), tsx.parent / image.get("source"), f"{source}"))

missing = defaultdict(list)
for kind, value, resolved, where in references:
    if not resolved.exists():
        missing[(kind, value)].append((where, resolved))

checked = len({(k, v) for k, v, _, _ in references})
print(f"{tmx_path}")
print(f"  {len(references)} file references, {checked} distinct")
if not missing:
    print("  all resolve on disk")
    sys.exit(0)

print(f"  {len(missing)} MISSING:")
for (kind, value), uses in sorted(missing.items()):
    print(f'      {kind} = "{value}"  ->  {uses[0][1].as_posix()}')
    for where, _ in uses:
        print(f"          used by {where}")
sys.exit(1)
