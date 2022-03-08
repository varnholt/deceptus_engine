#pragma once

#include <filesystem>
#include <Box2D/Box2D.h>
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxtileset.h"


struct GameDeserializeData
{
   std::filesystem::path _base_path;
   std::shared_ptr<b2World> _world;
   TmxObject* _tmx_object = nullptr;
   TmxObjectGroup* _tmx_object_group = nullptr;
   TmxLayer* _tmx_layer = nullptr;
   TmxTileSet* _tmx_tileset = nullptr;
};

