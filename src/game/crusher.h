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

      enum class State
      {
         Idle,
         Extract,
         Retract
      };

      Crusher(GameNode* parent = nullptr);

      void draw(sf::RenderTarget& target) override;
      void update(const sf::Time& dt) override;


      void setup(
         TmxObject* tmxObject,
         const std::shared_ptr<b2World>& world
      );


   private:

      void updateTransform();
      void setupBody(const std::shared_ptr<b2World>& world);

      void step(const sf::Time& dt);
      void updateState();
      void updateSpritePositions();

      Mode mMode = Mode::Interval;
      State mState = State::Idle;
      State mPreviousState = State::Idle;
      Alignment mAlignment = Alignment::PointsDown;
      float mAngle = 0.0f;

      b2Body* mBody = nullptr;
      sf::Vector2f mPixelPosition;
      sf::Vector2f mOffset;

      sf::Time mIdleTime;
      sf::Time mExtractionTime;
      sf::Time mRetractionTime;

      sf::Sprite mSpriteSpike;
      sf::Sprite mSpritePusher;
      sf::Sprite mSpriteMount;

      int32_t mInstanceId = 0;

      static sf::Texture mTexture;
      static int32_t mInstanceCounter;
};

