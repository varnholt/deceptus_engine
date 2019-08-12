#pragma once

#include "constants.h"
#include "fixturenode.h"
#include "Box2D/Box2D.h"
#include "SFML/Graphics.hpp"


class Bouncer : public FixtureNode
{

public:

   Bouncer(
      GameNode* parent,
      const std::shared_ptr<b2World>& world,
      float x,
      float y,
      float width,
      float height
   );

   void draw(sf::RenderTarget &window);
   void update(float dt);
   bool isPlayerAtBouncer();

   virtual ~Bouncer();
   void activate();
   b2Body *getBody() const;
   int getZ() const;
   void setZ(int z);


private:

   Alignment mAlignment = Alignment::PointsUp;
   b2Body* mBody = nullptr;
   b2Vec2 mPositionB2d;
   sf::Vector2f mPositionSf;
   b2PolygonShape mShapeBounds;
   b2PolygonShape mShapeSensor;

   sf::Texture mTexture;
   sf::Sprite mSprite;
   int mZ = 0;
   sf::Time mActivationTime;
   bool mPlayerAtBouncer = false;
   void updatePlayerAtBouncer();
};
