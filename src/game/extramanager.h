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

   std::vector<std::shared_ptr<InventoryItem>> mInventory;
   std::vector<std::shared_ptr<ExtraItem>> mExtras;

   std::shared_ptr<TileMap> mTilemap;


public:

   void load(TmxLayer *layer, TmxTileSet *tileSet);
   void collide(const sf::Rect<int> &playerRect);

   ExtraManager();
};

#endif // EXTRAMANAGER_H
