#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <SFML/Graphics.hpp>

#include "gamenode.h"

struct ExtraItem;
class GameNode;
struct GameDeserializeData;
struct InventoryItem;
class TileMap;
struct TmxLayer;
struct TmxTileSet;

class Extra : public GameNode
{
public:
   struct ExtraItem : public GameNode
   {

      ExtraItem(GameNode* parent = nullptr);

      bool _active = true;
      std::string _name;
      std::string _sample;
      sf::Sprite _sprite;
      std::shared_ptr<sf::Texture> _texture;
      sf::FloatRect _rect;
      int32_t _z = 0;
   };

   using ExtraCollback = std::function<void(const std::string&)>;

   void deserialize(GameNode* parent, const GameDeserializeData& data);
   void collide(const sf::FloatRect& player_rect);
   void resetExtras();

   Extra(GameNode* parent = nullptr);

   std::vector<std::shared_ptr<ExtraItem>> _extra_items;
   std::vector<ExtraCollback> _callbacks;
};
