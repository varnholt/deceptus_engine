#ifndef EXTRAMANAGER_H
#define EXTRAMANAGER_H

#include <memory>
#include <vector>

#include <SFML/Graphics.hpp>

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

   ExtraManager() = default;

   std::vector<std::shared_ptr<InventoryItem>> mInventory;
   std::vector<std::shared_ptr<ExtraItem>> mExtras;

   std::shared_ptr<TileMap> mTilemap;
};

#endif // EXTRAMANAGER_H
