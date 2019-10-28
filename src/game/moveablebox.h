#pragma once

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

#include "gamenode.h"

struct TmxObject;


class MoveableBox : public GameNode
{
   public:
      MoveableBox(GameNode* node);
      void draw(sf::RenderTarget& window);
      void update(const sf::Time& dt);
      void setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& world);
      int32_t getZ() const;

   private:

      std::shared_ptr<sf::Texture> mTexture;
      sf::Sprite mSprite;
      sf::Vector2f mSize;
      int32_t mZ = 0;
};

