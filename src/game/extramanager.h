#pragma once

#include <memory>
#include <vector>

#include <SFML/Graphics.hpp>

#include "constants.h"

struct ExtraItem;
class GameNode;
struct GameDeserializeData;
struct InventoryItem;
class TileMap;
struct TmxLayer;
struct TmxTileSet;

class ExtraManager
{
public:
   [[deprecated]] void load(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TmxTileSet>& tileset);
   void deserialize(GameNode* parent, const GameDeserializeData& data);

   void collide(const sf::FloatRect& player_rect);

   void resetExtras();

   ExtraManager() = default;

   std::vector<std::shared_ptr<ExtraItem>> _extras;

   std::shared_ptr<TileMap> _tilemap;
};
