"""Generates the harpoon test level tmx.

Layout: one horizontal band of air between a rock ceiling and a base floor, kept short enough
(10 tiles) that the ceiling is always within harpoon range. Platforms hang in that band with gaps
too wide to jump, so the only way across is a swing. Stalactites give the rope corners to wrap
around.
"""

import pathlib

WIDTH_TL = 120
HEIGHT_TL = 22

CEILING_BOTTOM_TL = 5  # rows 0..5 are rock
FLOOR_TOP_TL = 16  # rows 16..21 are the base floor
PLATFORM_TOP_TL = 12  # platform surface, 4 tiles above the base floor
PLATFORM_BOTTOM_TL = 15

SOLID_TILE_ID = 64  # first tileset tile carrying a full 24x24 collision box
FIRST_GID = 1

PLATFORMS = [(2, 14), (27, 38), (55, 66), (88, 104)]
STALACTITES = [(20, 21, 9), (72, 73, 10), (80, 81, 10)]  # x_from, x_to, y_to

# the base floor is continuous everywhere else, so this is the one spot that cannot be walked
# across. it is only three tiles deep, which keeps the ceiling inside harpoon range from the bottom
PIT_X_FROM = 106
PIT_X_TO = 114
PIT_BOTTOM_TL = 18

# a flooded pit, so "entering water drops the rope" can be tried out and driven by a script
WATER_X_FROM = 44
WATER_X_TO = 52
WATER_BOTTOM_TL = 18

# the shipped levels drive the atmosphere layer from physics_tiles.tsx and keep it visible="0";
# here it stays visible on purpose, the cyan marker is how the pool is found in a test level
ATMOSPHERE_FIRST_GID = 4097  # the diffuse tileset owns 1..4096
ATMOSPHERE_TILE_WATER_FULL = 48  # AtmosphereTile in constants.h
ATMOSPHERE_TILE_WATER_TOP = 49


def is_solid(x_tl: int, y_tl: int) -> bool:
    if y_tl <= CEILING_BOTTOM_TL:
        return True
    if y_tl >= FLOOR_TOP_TL:
        if PIT_X_FROM <= x_tl <= PIT_X_TO and y_tl <= PIT_BOTTOM_TL:
            return False
        if WATER_X_FROM <= x_tl <= WATER_X_TO and y_tl <= WATER_BOTTOM_TL:
            return False
        return True
    if x_tl <= 1 or x_tl >= WIDTH_TL - 2:
        return True
    for x_from, x_to in PLATFORMS:
        if x_from <= x_tl <= x_to and PLATFORM_TOP_TL <= y_tl <= PLATFORM_BOTTOM_TL:
            return True
    for x_from, x_to, y_to in STALACTITES:
        if x_from <= x_tl <= x_to and CEILING_BOTTOM_TL < y_tl <= y_to:
            return True
    return False


def build_csv() -> str:
    rows = []
    for y_tl in range(HEIGHT_TL):
        row = [str(SOLID_TILE_ID + FIRST_GID if is_solid(x_tl, y_tl) else 0) for x_tl in range(WIDTH_TL)]
        rows.append(",".join(row))
    return ",\n".join(rows)


def water_tile(x_tl: int, y_tl: int) -> int:
    """The atmosphere layer is what makes the player count as being in water."""
    if not (WATER_X_FROM <= x_tl <= WATER_X_TO):
        return 0
    if not (FLOOR_TOP_TL <= y_tl <= WATER_BOTTOM_TL):
        return 0

    tile_id = ATMOSPHERE_TILE_WATER_TOP if y_tl == FLOOR_TOP_TL else ATMOSPHERE_TILE_WATER_FULL
    return ATMOSPHERE_FIRST_GID + tile_id


def build_atmosphere_csv() -> str:
    rows = []
    for y_tl in range(HEIGHT_TL):
        rows.append(",".join(str(water_tile(x_tl, y_tl)) for x_tl in range(WIDTH_TL)))
    return ",\n".join(rows)


TEMPLATE = """<?xml version="1.0" encoding="UTF-8"?>
<map version="1.8" tiledversion="1.8.4" orientation="orthogonal" renderorder="right-down" width="{width}" height="{height}" tilewidth="24" tileheight="24" infinite="0" nextlayerid="3" nextobjectid="1">
 <tileset firstgid="{first_gid}" source="catacombs-level-diffuse.tsx"/>
 <tileset firstgid="{atmosphere_first_gid}" source="physics_tiles.tsx"/>
 <layer id="1" name="level" width="{width}" height="{height}">
  <properties>
   <property name="z" type="int" value="24"/>
  </properties>
  <data encoding="csv">
{csv}
</data>
 </layer>
 <layer id="2" name="atmosphere" width="{width}" height="{height}">
  <properties>
   <property name="z" type="int" value="18"/>
  </properties>
  <data encoding="csv">
{atmosphere_csv}
</data>
 </layer>
</map>
"""

output_path = pathlib.Path("data/level-harpoon_test/level.tmx")
output_path.write_text(
    TEMPLATE.format(
        width=WIDTH_TL,
        height=HEIGHT_TL,
        first_gid=FIRST_GID,
        atmosphere_first_gid=ATMOSPHERE_FIRST_GID,
        csv=build_csv(),
        atmosphere_csv=build_atmosphere_csv(),
    ),
    encoding="utf-8",
)
print(f"wrote {output_path} ({output_path.stat().st_size} bytes)")

level_json_path = pathlib.Path("data/level-harpoon_test/level.json")
level_json_path.write_text(
    '{\n  "filename": "data/level-harpoon_test/level.tmx",\n  "startposition": [6, 10]\n}\n',
    encoding="utf-8",
)
print(f"wrote {level_json_path}")
