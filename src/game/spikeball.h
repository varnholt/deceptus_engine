#pragma once

#include <SFML/Graphics.hpp>

#include "Box2D/Box2D.h"

#include "gamenode.h"


class SpikeBall : public GameNode
{
   public:
      SpikeBall(GameNode* parent);

      void draw(sf::RenderTarget& window);
      void update(const sf::Time& dt);


      void setup(const std::shared_ptr<b2World>& world);

      int32_t getZ() const;
      void setZ(const int32_t& z);


   private:
      b2BodyDef mGroundDef;
      b2Body* mGround = nullptr;
      b2EdgeShape mGroundShape;

      b2RevoluteJointDef mJointDef;
      b2PolygonShape mChainElementShape;
      b2FixtureDef mChainElementFixtureDef;
      float mChainElementLength = 25.0f;
      std::vector<b2Body*> mChainElements;

      int32_t mZ = 0;
};

