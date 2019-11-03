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

      void draw(sf::RenderTarget& window) override;
      void update(const sf::Time& dt) override;

      void setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& world);

   private:

      void setupBody(const std::shared_ptr<b2World>& world);
      void setupTransform();

      std::shared_ptr<sf::Texture> mTexture;
      sf::Sprite mSprite;
      sf::Vector2f mSize;
      b2Body* mBody = nullptr;
};

