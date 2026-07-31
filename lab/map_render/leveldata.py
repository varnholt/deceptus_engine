"""Reads the level sources the in-game map needs: collision mesh, rooms and marker objects.

This mirrors what the engine already has in memory at runtime:
  - layer_level_solid.obj  -> Level::parseObj / the box2d chains
  - objectgroup "rooms"    -> Level::_rooms
  - mechanism objectgroups -> GameMechanismRegistry
"""

import xml.etree.ElementTree as ElementTree
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class Rect:
    x: float
    y: float
    width: float
    height: float

    @property
    def right(self) -> float:
        return self.x + self.width

    @property
    def bottom(self) -> float:
        return self.y + self.height


@dataclass
class SubRoom:
    name: str
    rect: Rect


@dataclass
class Room:
    """One room group, made of one or more rectangular sub-rooms (grouped via the 'group' property)."""

    identifier: int
    group_name: str | None
    sub_rooms: list[SubRoom] = field(default_factory=list)


@dataclass
class Marker:
    kind: str
    name: str
    rect: Rect

    @property
    def center(self) -> tuple[float, float]:
        return (self.rect.x + self.rect.width * 0.5, self.rect.y + self.rect.height * 0.5)


@dataclass
class LevelData:
    width_tl: int
    height_tl: int
    tile_width_px: int
    tile_height_px: int
    mesh_faces: list[list[tuple[float, float]]]
    rooms: list[Room]
    markers: list[Marker]

    @property
    def width_px(self) -> int:
        return self.width_tl * self.tile_width_px

    @property
    def height_px(self) -> int:
        return self.height_tl * self.tile_height_px


def read_obj(path: Path) -> list[list[tuple[float, float]]]:
    """Reads the optimized collision mesh. Every face is one closed polygon loop in world pixels."""
    vertices: list[tuple[float, float]] = []
    faces: list[list[tuple[float, float]]] = []

    with path.open("r", encoding="utf-8") as obj_file:
        for line in obj_file:
            if line.startswith("v "):
                parts = line.split()
                vertices.append((float(parts[1]), float(parts[2])))
            elif line.startswith("f "):
                indices = [int(token.split("/")[0]) for token in line.split()[1:]]
                faces.append([vertices[index - 1] for index in indices])

    return faces


# object groups that produce a map marker, mapped to the marker kind used by the renderer
MARKER_GROUPS = {
    "checkpoints": "checkpoint",
    "portals": "portal",
    "doors": "door",
    "extras": "extra",
    "treasure_chests": "treasure_chest",
    "level_transitions": "level_transition",
}


def _read_properties(object_node: ElementTree.Element) -> dict[str, str]:
    properties: dict[str, str] = {}
    properties_node = object_node.find("properties")
    if properties_node is None:
        return properties
    for property_node in properties_node.findall("property"):
        properties[property_node.get("name", "")] = property_node.get("value", "")
    return properties


def _read_rooms(object_group: ElementTree.Element, view_width: int = 640, view_height: int = 360) -> list[Room]:
    """Groups room rectangles the same way Room::deserialize does: by the optional 'group' property.

    Rectangles smaller than the screen are rejected, just like Room::deserialize does.
    """
    rooms: list[Room] = []
    rooms_by_group: dict[str, Room] = {}
    next_identifier = 0

    for object_node in object_group.findall("object"):
        name = object_node.get("name", "")

        if name.startswith("enter_area"):
            continue

        rect = Rect(
            float(object_node.get("x", "0")),
            float(object_node.get("y", "0")),
            float(object_node.get("width", "0")),
            float(object_node.get("height", "0")),
        )

        if rect.width < view_width or rect.height < view_height:
            continue

        group_name = _read_properties(object_node).get("group")
        sub_room = SubRoom(name, rect)

        if group_name is not None:
            existing_room = rooms_by_group.get(group_name)
            if existing_room is not None:
                existing_room.sub_rooms.append(sub_room)
                continue

        room = Room(next_identifier, group_name, [sub_room])
        next_identifier += 1
        rooms.append(room)

        if group_name is not None:
            rooms_by_group[group_name] = room

    return rooms


def load(level_directory: Path, tmx_filename: str, obj_filename: str = "layer_level_solid.obj") -> LevelData:
    tree = ElementTree.parse(level_directory / tmx_filename)
    map_node = tree.getroot()

    rooms: list[Room] = []
    markers: list[Marker] = []

    for object_group in map_node.findall("objectgroup"):
        group_name = object_group.get("name", "")
        offset_x_px = float(object_group.get("offsetx", "0"))
        offset_y_px = float(object_group.get("offsety", "0"))

        if group_name == "rooms":
            rooms = _read_rooms(object_group)
            continue

        marker_kind = MARKER_GROUPS.get(group_name)
        if marker_kind is None:
            continue

        for object_node in object_group.findall("object"):
            markers.append(
                Marker(
                    marker_kind,
                    object_node.get("name", ""),
                    Rect(
                        float(object_node.get("x", "0")) + offset_x_px,
                        float(object_node.get("y", "0")) + offset_y_px,
                        float(object_node.get("width", "0")),
                        float(object_node.get("height", "0")),
                    ),
                )
            )

    return LevelData(
        width_tl=int(map_node.get("width", "0")),
        height_tl=int(map_node.get("height", "0")),
        tile_width_px=int(map_node.get("tilewidth", "24")),
        tile_height_px=int(map_node.get("tileheight", "24")),
        mesh_faces=read_obj(level_directory / obj_filename),
        rooms=rooms,
        markers=markers,
    )
