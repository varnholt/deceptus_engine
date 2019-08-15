#pragma once

#include <SFML/Graphics.hpp>

#include "Box2D/Box2D.h"

#include "gamenode.h"


class SpikeBall : public GameNode
{
   public:
      SpikeBall(GameNode* parent);

      void draw(sf::RenderTarget& window);
      void update(const sf::Time& dt);


      void setup(const std::shared_ptr<b2World>& world);

};

