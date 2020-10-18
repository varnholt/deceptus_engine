#pragma once

#include "constants.h"
#include "gamemechanism.h"
#include "gamenode.h"

#include <filesystem>

#include <Box2D/Box2D.h>


struct TmxObject;

class Stomper : public GameMechanism, public GameNode
{
   public:

      enum class Mode
      {
         Interval,
         Distance
      };

      Stomper(GameNode* parent = nullptr);

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
};

