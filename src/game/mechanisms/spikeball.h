#pragma once

#include <SFML/Graphics.hpp>

#include "Box2D/Box2D.h"

#include "gamemechanism.h"
#include "gamenode.h"

struct TmxObject;


class SpikeBall : public GameMechanism, public GameNode
{
   public:

      struct SpikeConfig
      {
         // factor to control the push force when ball moves from right to left
         float pushFactor = 0.625f;

         // number of points retrieved from the given spline
         int32_t splinePointCount = 25;

         // chain element setup
         int32_t chainElementCount = 10;
         float chainElementDistance = 0.3f;
         float chainElementWidth = 0.06f;
         float chainElementHeight = 0.0125f;
      };

      SpikeBall(GameNode* parent);

      void draw(sf::RenderTarget& window) override;
      void update(const sf::Time& dt) override;

      void setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& world);

      sf::Vector2i getPixelPosition() const;
      void setPixelPosition(const sf::Vector2i& pixelPosition);


   private:

      void drawChain(sf::RenderTarget& window);

      std::shared_ptr<sf::Texture> mTexture;
      sf::Sprite mSpikeSprite;
      sf::Sprite mBoxSprite;
      sf::Sprite mChainElementA;
      sf::Sprite mChainElementB;

      sf::Vector2i mPixelPosition;

      b2BodyDef mGroundDef;
      b2Body* mGround = nullptr;
      b2EdgeShape mGroundShape;

      b2RevoluteJointDef mJointDef;
      b2PolygonShape mChainElementShape;
      b2FixtureDef mChainElementFixtureDef;
      std::vector<b2Body*> mChainElements;

      b2Body* mBallBody = nullptr;
      b2CircleShape mBallShape;
      b2BodyDef mBallBodyDef;
      b2FixtureDef mBallFixtureDef;

      float mAngle = 0.0f;
      SpikeConfig mConfig;
};

