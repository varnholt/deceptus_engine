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

   void draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   using ExtraCallback = std::function<void(const std::string&)>;

   bool _active = true;
   std::string _name;
   std::optional<std::string> _sample;
   sf::Sprite _sprite;
   std::shared_ptr<sf::Texture> _texture;
   sf::FloatRect _rect;
   int32_t _z = 0;
   std::vector<ExtraCallback> _callbacks;
};
