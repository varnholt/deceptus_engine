#pragma once

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


   void processPlayerHeadSensorContactEnd(auto contact_fixture_b);

protected:

   bool isPlayer(FixtureNode* obj) const;


private:

   GameContactListener() = default;

   void processBeginContact(
      b2Contact* contact,
      b2Fixture* contact_fixture_a,
      b2Fixture* contact_fixture_b,
      FixtureNode* fixture_node_a,
      FixtureNode* fixture_node_b,
      void* fixture_user_data_b
   );

   void processEndContact(
      b2Contact* contact,
      FixtureNode* fixture_node_a,
      FixtureNode* fixture_node_b,
      b2Fixture* contact_fixture_b
   );

   void processPostSolve(FixtureNode* node, float impulse);

   void processBouncerContactBegin(FixtureNode* fixture_node);
   void processBubbleCubeContactBegin(FixtureNode* fixture_node);
   void processCollapsingPlatformContactBegin(FixtureNode* fixture_node);
   void processCrusherContactBegin(FixtureNode* fixture_node);
   void processDeadlyContactBegin(FixtureNode* fixture_node);
   void processEnemyContactBegin(FixtureNode* fixture_node_a, FixtureNode* fixture_node_b);
   void processMovingPlatformContactBegin(b2Fixture* fixture, void* fixture_user_data);
   void processOneWayWallContactBegin(b2Contact* contact, b2Fixture* fixture);
   void processPlayerContactBegin();
   void processPlayerFootSensorContactBegin(b2Fixture* fixture);
   void processPlayerHeadSensorContactBegin(b2Fixture* fixture);
   void processPlayerLeftArmSensorContactBegin(b2Fixture* fixture);
   void processPlayerRightArmSensorContactBegin(b2Fixture* fixture);
   void processProjectileContactBegin(FixtureNode* fixture_node_a, FixtureNode* fixture_node_b);

   void processBubbleCubeContactEnd(FixtureNode* fixture_node);
   void processCollapsingPlatformContactEnd(FixtureNode* fixture_node);
   void processCrusherContactEnd(FixtureNode* fixture_node);
   void processDeadlyContactEnd(FixtureNode* fixture_node);
   void processMovingPlatformContactEnd();
   void processOneWayWallContactEnd(b2Contact* contact);
   void processPlayerContactEnd();
   void processPlayerFootSensorContactEnd(b2Fixture* fixture);
   void processPlayerLeftArmSensorContactEnd(b2Fixture* contact_fixture);
   void processPlayerRightArmSensorContactEnd(b2Fixture* contact_fixture);

   void processPostSolveImpulse(float impulse);
   void processPostSolveProjectile(FixtureNode* node, float impulse);

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

