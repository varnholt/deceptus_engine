#pragma once

#include <memory>
#include <vector>

#include <SFML/Graphics.hpp>

#include "constants.h"
#include "gamenode.h"

struct ExtraItem;
class GameNode;
struct GameDeserializeData;
struct InventoryItem;
class TileMap;
struct TmxLayer;
struct TmxTileSet;

class Extra
{
public:
   struct ExtraItem : public GameNode
   {
      enum class ExtraSpriteIndex
      {
         Coin = 0,
         Cherry = 16,
         Banana = 32,
         Apple = 48,
         KeyRed = 64,
         KeyOrange = 65,
         KeyBlue = 66,
         KeyGreen = 67,
         KeyYellow = 68,
         Dash = 80,
         Invalid = 0xffff
      };

      ExtraItem(GameNode* parent = nullptr);

      bool _active = true;
      sf::Vector2u _sprite_offset;
      sf::Vector2f _position;
      ExtraSpriteIndex _type = ExtraSpriteIndex::Invalid;
   };

   [[deprecated]] void load(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TmxTileSet>& tileset);
   void deserialize(GameNode* parent, const GameDeserializeData& data);
   void collide(const sf::FloatRect& player_rect);
   void resetExtras();

   Extra() = default;

   std::vector<std::shared_ptr<ExtraItem>> _extra_items;
   std::shared_ptr<TileMap> _tilemap;
};
