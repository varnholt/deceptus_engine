"""cross-check every identifier level.lua resolves by name against a tmx map."""

import re
import sys
import xml.etree.ElementTree as ElementTree
from collections import defaultdict

tmx_path, lua_path = sys.argv[1], sys.argv[2]

tree = ElementTree.parse(tmx_path)
objects_by_layer = defaultdict(set)
for group in tree.iter("objectgroup"):
    for map_object in group.iter("object"):
        if map_object.get("name"):
            objects_by_layer[group.get("name")].add(map_object.get("name"))
image_layers = {el.get("name") for el in tree.iter("imagelayer")}
all_object_names = {n for names in objects_by_layer.values() for n in names}

lua = open(lua_path, encoding="utf-8").read()
# strip comments so commented-out calls are not reported
lua_active = "\n".join(re.sub(r"--.*$", "", line) for line in lua.splitlines())

failures, checks = [], 0

# setMechanismEnabled("name", bool, "layer") / setMechanismVisible(...)
for call, name, layer in re.findall(
    r'(setMechanism(?:Enabled|Visible))\(\s*"([^"]+)"\s*,\s*\w+\s*,\s*"([^"]+)"\s*\)', lua_active
):
    checks += 1
    pool = image_layers if layer == "imagelayers" else objects_by_layer.get(layer, set())
    if name not in pool:
        failures.append(f'{call}("{name}", ..., "{layer}") -> no such object in layer "{layer}"')

# showDialogue("name")
for name in re.findall(r'showDialogue\(\s*"([^"]+)"\s*\)', lua_active):
    checks += 1
    if name not in objects_by_layer.get("dialogues", set()):
        failures.append(f'showDialogue("{name}") -> no such object in layer "dialogues"')

# object_id == "name"  (event dispatch)
for name in re.findall(r'object_id\s*==\s*"([^"]+)"', lua_active):
    checks += 1
    if name not in all_object_names:
        failures.append(f'object_id == "{name}" -> no object with that name anywhere in the map')

print(f"{tmx_path}")
print(f"  checked {checks} name lookups from {lua_path}")
if failures:
    print(f"  {len(failures)} UNRESOLVED:")
    for failure in sorted(set(failures)):
        print(f"      {failure}")
else:
    print("  all resolved")
sys.exit(0)
