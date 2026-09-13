"""walk every revision of a catacombs map and report how locked_box was wired.

for each revision: locked_box's spawn-related properties, plus any extras layer
object within a radius of it, so a reward that used to sit there shows up.
"""

import subprocess
import sys
import xml.etree.ElementTree as ElementTree

repo, path = sys.argv[1], sys.argv[2]
RADIUS = 700.0


def run(args):
    return subprocess.run(args, cwd=repo, capture_output=True, check=False)


revisions = run(["git", "log", "--reverse", "--format=%h\t%ad\t%s", "--date=short", "--all", "--", path])
rows = [line.split("\t", 2) for line in revisions.stdout.decode("utf-8", "replace").splitlines() if line.strip()]

previous_signature = None
for sha, date, subject in rows:
    blob = run(["git", "show", f"{sha}:{path}"])
    if blob.returncode != 0:
        continue
    try:
        root = ElementTree.fromstring(blob.stdout.decode("utf-8", "replace"))
    except ElementTree.ParseError as error:
        print(f"{sha} {date}  <unparseable: {error}>")
        continue

    chest = None
    extras = []
    for group in root.iter("objectgroup"):
        layer = group.get("name")
        for map_object in group.iter("object"):
            name = map_object.get("name") or ""
            if layer == "treasure_chests" and name == "locked_box":
                chest = (
                    map_object,
                    {p.get("name"): p.get("value") for p in map_object.iter("property")},
                )
            elif layer == "extras":
                extras.append(map_object)

    if chest is None:
        signature = "no locked_box object"
    else:
        map_object, props = chest
        chest_x, chest_y = float(map_object.get("x")), float(map_object.get("y"))
        spawn = props.get("spawn_extra", "-")
        offset = (props.get("spawn_offset_x", "-"), props.get("spawn_offset_y", "-"))
        near = []
        for extra in extras:
            extra_x, extra_y = float(extra.get("x")), float(extra.get("y"))
            distance = ((extra_x - chest_x) ** 2 + (extra_y - chest_y) ** 2) ** 0.5
            if distance <= RADIUS:
                near.append(f'{extra.get("name") or "<unnamed>"}(id {extra.get("id")}, {distance:.0f}px)')
        signature = (
            f"chest@({chest_x:.0f},{chest_y:.0f}) item_required={props.get('item_required','-')} "
            f"spawn_extra={spawn} offset={offset[0]},{offset[1]} nearby_extras=[{', '.join(near) or 'none'}]"
        )

    if signature != previous_signature:
        print(f"{sha} {date}  {subject[:52]}")
        print(f"    {signature}")
        previous_signature = signature

print(f"\n({len(rows)} revisions scanned; only changes shown)")
