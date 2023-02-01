#pragma once

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>
#include <optional>

#include "gamedeserializedata.h"
#include "gamemechanism.h"
#include "gamenode.h"

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

private:
   void setupBody(const std::shared_ptr<b2World>& world);
   void setupTransform();

   std::shared_ptr<sf::Texture> _texture;
   sf::Sprite _sprite;
   sf::Vector2f _size;
   b2Body* _body = nullptr;
   std::optional<int32_t> _pushing_sample;
};
