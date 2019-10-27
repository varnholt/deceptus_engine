#pragma once

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

#include "gamenode.h"

class TmxObject;


class MoveableBox : public GameNode
{
   public:
      MoveableBox(GameNode* node);
      void draw(sf::RenderTarget& window);
      void update(const sf::Time& dt);
      void setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& world);

   private:

      std::shared_ptr<sf::Texture> mTexture;
      std::vector<sf::Sprite> mSprites;
      sf::Vector2f mSize;
};

