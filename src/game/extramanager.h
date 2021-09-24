#pragma once

#include <memory>
#include <vector>

#include <SFML/Graphics.hpp>

#include "constants.h"

struct ExtraItem;
struct InventoryItem;
class TileMap;
struct TmxLayer;
struct TmxTileSet;


class ExtraManager
{
public:

   void load(TmxLayer *layer, TmxTileSet *tileSet);
   void collide(const sf::Rect<int32_t>& playerRect);

   void resetExtras();

   ExtraManager() = default;

   std::vector<std::shared_ptr<ExtraItem>> _extras;

   std::shared_ptr<TileMap> _tilemap;
};

