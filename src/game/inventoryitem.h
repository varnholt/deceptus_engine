#ifndef INVENTORYITEM_H
#define INVENTORYITEM_H

#include "constants.h"
#include <SFML/Graphics.hpp>

struct InventoryItem
{
  InventoryItem() = default;

  ItemType mType;
  sf::Sprite mSprite;
  sf::Texture mTexture;
};

#endif // INVENTORYITEM_H
