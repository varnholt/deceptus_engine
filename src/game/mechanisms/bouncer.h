#pragma once

#include "constants.h"
#include "fixturenode.h"
#include "gamemechanism.h"
#include "Box2D/Box2D.h"
#include "SFML/Graphics.hpp"


class Bouncer : public FixtureNode, public GameMechanism
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

   ~Bouncer() override;

   void draw(sf::RenderTarget& window) override;
   void update(const sf::Time& dt) override;

   bool isPlayerAtBouncer();

   void activate();
   b2Body *getBody() const;


private:

   Alignment mAlignment = Alignment::PointsUp;
   b2Body* mBody = nullptr;
   b2Vec2 mPositionB2d;
   sf::Vector2f mPositionSf;
   b2PolygonShape mShapeBounds;
   b2PolygonShape mShapeSensor;

   std::shared_ptr<sf::Texture> mTexture;
   sf::Sprite mSprite;
   sf::IntRect mRect;
   sf::Time mActivationTime;
   bool mPlayerAtBouncer = false;
   void updatePlayerAtBouncer();
};
