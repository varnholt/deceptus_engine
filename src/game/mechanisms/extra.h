#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <SFML/Graphics.hpp>

#include "game/gamemechanism.h"
#include "game/gamenode.h"

struct ExtraItem;
class GameNode;
struct GameDeserializeData;
struct InventoryItem;
class TileMap;
struct TmxLayer;
struct TmxTileSet;

class Extra : public GameMechanism, public GameNode
{
public:
   Extra(GameNode* parent = nullptr);

   std::shared_ptr<Extra> deserialize(const GameDeserializeData& data);

   using ExtraCollback = std::function<void(const std::string&)>;

   static void collide(const sf::FloatRect& player_rect);
   static void resetExtras();
   static std::vector<std::shared_ptr<Extra>> _extra_items;
   static std::vector<ExtraCollback> _callbacks;

   virtual std::optional<sf::FloatRect> getBoundingBoxPx() override;

   bool _active = true;
   std::string _name;
   std::string _sample;
   sf::Sprite _sprite;
   std::shared_ptr<sf::Texture> _texture;
   sf::FloatRect _rect;
   int32_t _z = 0;
};
