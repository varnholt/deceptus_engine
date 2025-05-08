#pragma once

#include <box2d/box2d.h>
#include <SFML/Graphics.hpp>
#include <optional>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

struct TmxObject;

class MoveableBox : public GameMechanism, public GameNode
{
public:
   MoveableBox(GameNode* node);

   void preload() override;
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   void setup(const GameDeserializeData& data);

   struct Settings
   {
      float _density = 1.0f;
      float _friction = 0.0f;
      float _gravity_scale = 1.0f;
   };

private:
   void setupBody(const std::shared_ptr<b2World>& world);
   void setupTransform();

   std::shared_ptr<sf::Texture> _texture;
   std::unique_ptr<sf::Sprite> _sprite;
   sf::Vector2f _size;
   b2Body* _body = nullptr;
   std::optional<int32_t> _pushing_sample;
   Settings _settings;
};
