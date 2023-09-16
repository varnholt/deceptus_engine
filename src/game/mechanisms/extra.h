#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <SFML/Graphics.hpp>

#include "game/animationpool.h"
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

   void deserialize(const GameDeserializeData& data);

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
   std::vector<ExtraCallback> _callbacks;

   // animations (if used)

   // unclear if a probability is needed
   //
   // struct ExtraAnimationData
   // {
   //    float _probability{1.0f};
   //    std::shared_ptr<Animation> _animation;
   // };

   std::vector<std::shared_ptr<Animation>> _animations_main;
   std::shared_ptr<Animation> _animation_pickup;
   std::vector<std::shared_ptr<Animation>>::iterator _animations_main_it;
};
