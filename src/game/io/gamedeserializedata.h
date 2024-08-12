#pragma once

#include <box2d/box2d.h>
#include <filesystem>
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "framework/tmxparser/tmxtileset.h"

struct GameDeserializeData
{
   std::filesystem::path _base_path;
   std::shared_ptr<b2World> _world;
   std::shared_ptr<TmxObject> _tmx_object;
   std::shared_ptr<TmxObjectGroup> _tmx_object_group;
   std::shared_ptr<TmxLayer> _tmx_layer;
   std::shared_ptr<TmxTileSet> _tmx_tileset;
};
