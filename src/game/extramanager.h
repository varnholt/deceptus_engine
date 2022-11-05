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
   void load(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TmxTileSet>& tileset);
   void collide(const sf::FloatRect& player_rect);

   void resetExtras();

   ExtraManager() = default;

   std::vector<std::shared_ptr<ExtraItem>> _extras;

   std::shared_ptr<TileMap> _tilemap;
};
