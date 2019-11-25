#pragma once

#include "constants.h"
#include "gamemechanism.h"
#include "fixturenode.h"
#include "Box2D/Box2D.h"
#include "SFML/Graphics.hpp"


class ConveyorBelt : public FixtureNode, public GameMechanism
{

   public:

      ConveyorBelt(
         GameNode* parent,
         const std::shared_ptr<b2World>& world,
         float x,
         float y,
         float width,
         float height
      );

      b2Body *getBody() const;
      float getVelocity() const;
      void setVelocity(float velocity);

      static void update();
      static void processContact(b2Contact *contact);
      static void processFixtureNode(FixtureNode* fixtureNode, b2Body* collidingBody);


   private:

      b2Body* mBody = nullptr;
      b2Vec2 mPositionB2d;
      sf::Vector2f mPositionSf;
      b2PolygonShape mShapeBounds;

      sf::Texture mTexture;
      sf::Sprite mSprite;

      // bool mActive = true;
      float mVelocity = -0.2f;

      static std::vector<b2Body*> sBodiesOnBelt;
};

