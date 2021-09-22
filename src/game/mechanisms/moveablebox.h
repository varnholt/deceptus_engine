#pragma once

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

#include "gamemechanism.h"
#include "gamenode.h"

struct TmxObject;


class MoveableBox : public GameMechanism, public GameNode
{
   public:
      MoveableBox(GameNode* node);

      void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
      void update(const sf::Time& dt) override;

      void setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& world);

   private:

      void setupBody(const std::shared_ptr<b2World>& world);
      void setupTransform();

      std::shared_ptr<sf::Texture> _texture;
      sf::Sprite _sprite;
      sf::Vector2f _size;
      b2Body* _body = nullptr;
};

