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

   void resetInventory();
   void resetExtras();
   void resetKeys();
   void giveAllKeys();

   ExtraManager() = default;

   bool hasInventoryItem(ItemType itemType) const;

   std::vector<std::shared_ptr<ExtraItem>> mExtras;

   std::shared_ptr<TileMap> mTilemap;
};

