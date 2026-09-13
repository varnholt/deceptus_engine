"""compare two tmx maps, keyed on (objectgroup, object id) to survive id collisions."""

import sys
import xml.etree.ElementTree as ET
from collections import defaultdict


def load(path):
    tree = ET.parse(path)
    objects = {}
    raw_ids = defaultdict(list)
    names = defaultdict(list)
    for group in tree.iter("objectgroup"):
        layer = group.get("name", "<unnamed>")
        for obj in group.iter("object"):
            props = {p.get("name"): p.get("value") for p in obj.iter("property")}
            key = (layer, obj.get("id"))
            objects[key] = {
                "name": obj.get("name") or "",
                "x": obj.get("x"), "y": obj.get("y"),
                "width": obj.get("width"), "height": obj.get("height"),
                "props": props,
            }
            raw_ids[obj.get("id")].append((layer, obj.get("name") or ""))
            if obj.get("name"):
                names[obj.get("name")].append((layer, obj.get("id")))
    return objects, raw_ids, names


engine, engine_ids, engine_names = load(sys.argv[1])
artist, artist_ids, artist_names = load(sys.argv[2])

for label, ids in (("ENGINE", engine_ids), ("ARTIST", artist_ids)):
    dupes = {i: v for i, v in ids.items() if len(v) > 1}
    print(f"### DUPLICATE OBJECT IDS IN {label}: {len(dupes)}")
    for i, v in sorted(dupes.items(), key=lambda kv: int(kv[0])):
        print(f'    id {i}: ' + ", ".join(f'layer "{l}" name "{n}"' for l, n in v))
print()

print("### DUPLICATE NAMES (name used by >1 object) — engine vs artist")
edupes = {n: v for n, v in engine_names.items() if len(v) > 1}
adupes = {n: v for n, v in artist_names.items() if len(v) > 1}
print(f"    engine: {len(edupes)} names   artist: {len(adupes)} names")
print("    NEW duplicate names introduced by artist:")
for n in sorted(set(adupes) - set(edupes)):
    print(f'      "{n}": ' + ", ".join(f'layer "{l}" id {i}' for l, i in adupes[n]))
print("    duplicate names the engine had and artist resolved:")
for n in sorted(set(edupes) - set(adupes)):
    print(f'      "{n}": ' + ", ".join(f'layer "{l}" id {i}' for l, i in edupes[n]))
print()

print("### OBJECTS ONLY IN ENGINE (artist removed / engine added)")
for key in sorted(set(engine) - set(artist), key=lambda k: (k[0], int(k[1]))):
    o = engine[key]
    print(f'    layer "{key[0]}" id {key[1]:>5} name "{o["name"]}" @({o["x"]},{o["y"]})')
print()

print("### OBJECTS ONLY IN ARTIST (new content)")
for key in sorted(set(artist) - set(engine), key=lambda k: (k[0], int(k[1]))):
    o = artist[key]
    print(f'    layer "{key[0]}" id {key[1]:>5} name "{o["name"]}" @({o["x"]},{o["y"]}) props={o["props"]}')
print()

print("### RENAMES (same layer+id, different name)")
for key in sorted(set(engine) & set(artist), key=lambda k: (k[0], int(k[1]))):
    if engine[key]["name"] != artist[key]["name"]:
        print(f'    layer "{key[0]}" id {key[1]:>5}: engine="{engine[key]["name"]}"  artist="{artist[key]["name"]}"')
