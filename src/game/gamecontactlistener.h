#pragma once


// box2d
#include "Box2D/Box2D.h"

#include <vector>

class FixtureNode;

class GameContactListener : public b2ContactListener
{

public:

   int32_t getPlayerHeadContactCount() const;
   int32_t getPlayerFootContactCount() const;
   int32_t getPlayerArmLeftContactCount() const;
   int32_t getPlayerArmRightContactCount() const;
   int32_t getPlayerContactCount() const;

   int32_t getDeadlyContactCount() const;
   int32_t getMovingPlatformContactCount() const;

   bool isPlayerSmashed() const;

   void BeginContact(b2Contact *contact) override;
   void EndContact(b2Contact *contact) override;

   void PreSolve(b2Contact *contact, const b2Manifold *oldManifold) override;
   void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override;

   void debug();
   void reset();

   static GameContactListener& getInstance();


protected:

   bool isPlayer(FixtureNode* obj) const;
   bool isBubbleCube(FixtureNode* obj) const;


private:

   GameContactListener() = default;

   void processOneSidedWalls(b2Contact* contact, b2Fixture* playerFixture, b2Fixture* platformFixture);
   void processImpulse(float impulse);

   int32_t _count_foot_contacts = 0;
   int32_t _count_head_contacts = 0;
   int32_t _count_player_contacts = 0;
   int32_t _count_arm_left_contacts = 0;
   int32_t _count_arm_right_contacts = 0;
   int32_t _count_deadly_contacts = 0;
   int32_t _count_moving_platform_contacts = 0;
   int32_t _count_bouncer_cycles = 5;
   bool _smashed = false;
};

