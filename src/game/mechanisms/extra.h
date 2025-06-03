#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <SFML/Graphics.hpp>

#include "game/animation/animationpool.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

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

   bool deserialize(const GameDeserializeData& data);

   void draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;
   void spawn();

   using ExtraCallback = std::function<void(const std::string&)>;

   bool _active{true};
   bool _spawn_required{false};
   bool _spawned{false};
   std::string _name;
   std::optional<std::string> _sample;
   std::unique_ptr<sf::Sprite> _sprite;
   std::shared_ptr<sf::Texture> _texture;
   sf::FloatRect _rect;
   std::vector<ExtraCallback> _callbacks;
   bool _requires_button_press{false};

   std::vector<std::shared_ptr<Animation>> _animations_main;
   std::shared_ptr<Animation> _animation_spawn;
   std::shared_ptr<Animation> _animation_pickup;
   std::vector<std::shared_ptr<Animation>>::iterator _animations_main_it;
};
