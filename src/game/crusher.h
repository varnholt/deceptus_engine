#pragma once

#include "constants.h"
#include "gamemechanism.h"
#include "gamenode.h"

#include <array>
#include <filesystem>

#include <Box2D/Box2D.h>


struct TmxObject;

class Crusher : public GameMechanism, public GameNode
{
   public:

      enum class Mode
      {
         Interval,
         Distance
      };

      Crusher(GameNode* parent = nullptr);

      void draw(sf::RenderTarget& target) override;
      void update(const sf::Time& dt) override;


      void setup(
         TmxObject* tmxObject,
         const std::shared_ptr<b2World>& world
      );


   private:

      void setupTransform();
      void setupBody(const std::shared_ptr<b2World>& world);

      b2Body* mBody = nullptr;
      sf::Vector2f mPixelPosition;
      Mode mMode = Mode::Distance;
      Alignment mAlignment = Alignment::PointsDown;

      sf::Sprite mSpriteSpike;
      sf::Sprite mSpritePusher;
      sf::Sprite mSpriteMount;

      static sf::Texture mTexture;
};

