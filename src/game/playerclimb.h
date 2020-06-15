#pragma once

#include <Box2D/Box2D.h>
#include <functional>

class b2Joint;
class b2ChainShape;

struct PlayerClimb
{
   PlayerClimb() = default;

   void update(b2Body* body, int32_t keysPressed, bool inAir);
   void removeClimbJoint();
   bool isClimbableEdge(b2ChainShape* shape, int currIndex);
   bool edgeMatchesMovement(const b2Vec2 &edgeDir);
   bool isClimbing() const;
   b2Joint* mClimbJoint = nullptr;
   int32_t mKeysPressed = 0;
};

