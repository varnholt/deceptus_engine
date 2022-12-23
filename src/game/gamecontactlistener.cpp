// base
#include "gamecontactlistener.h"

// game
#include "constants.h"
#include "fixturenode.h"
#include "framework/tools/log.h"
#include "luanode.h"
#include "mechanisms/bouncer.h"
#include "mechanisms/bubblecube.h"
#include "mechanisms/collapsingplatform.h"
#include "mechanisms/conveyorbelt.h"
#include "onewaywall.h"
#include "player/player.h"
#include "projectile.h"

#include <iostream>

// http://www.iforce2d.net/b2dtut/collision-anatomy
//
// TODO: pass collision normal to projectile detonation
//       so animation can be aligned to detonation angle.

int32_t GameContactListener::getPlayerFootContactCount() const
{
   return _count_foot_contacts;
}

int32_t GameContactListener::getDeadlyContactCount() const
{
   return _count_deadly_contacts;
}

bool GameContactListener::isPlayer(FixtureNode* obj) const
{
   if (!obj)
   {
      return false;
   }

   return dynamic_cast<Player*>(obj->getParent());
}

bool GameContactListener::isEnemy(FixtureNode* obj) const
{
   if (!obj)
   {
      return false;
   }

   return dynamic_cast<LuaNode*>(obj->getParent());
}

void GameContactListener::processProjectileContactBegin(FixtureNode* fixture_node_a, FixtureNode* fixture_node_b)
{
   const auto damage = std::get<int32_t>(fixture_node_a->getProperty("damage"));

   if (isPlayer(fixture_node_b))
   {
      Player::getCurrent()->damage(damage);
   }
   else if (fixture_node_b && fixture_node_b->getType() == ObjectTypeEnemy)
   {
      auto lua_node = dynamic_cast<LuaNode*>(fixture_node_b->getParent());
      if (lua_node != nullptr)
      {
         lua_node->luaHit(damage);
      }
   }

   auto projectile = dynamic_cast<Projectile*>(fixture_node_a);

   // if it's an arrow, let postsolve handle it. if the impulse is not
   // hard enough, the arrow should just fall on the ground
   if (!projectile->isSticky())
   {
      projectile->setScheduledForRemoval(true);
   }
}

void GameContactListener::processMovingPlatformContactBegin(b2Fixture* fixture, void* fixture_user_data)
{
   // check if platform smashes the player
   auto fixture_node = static_cast<FixtureNode*>(fixture_user_data);
   if (fixture_node && fixture_node->getType() == ObjectType::ObjectTypePlayerHeadSensor)
   {
      if (Player::getCurrent()->isOnGround())
      {
         _smashed = true;
      }
   }

   auto platform_body = fixture->GetBody();
   Player::getCurrent()->setPlatformBody(platform_body);

   _count_moving_platform_contacts++;
}

void GameContactListener::processCrusherContactBegin(FixtureNode* fixture_node)
{
   if (!isPlayer(fixture_node))
   {
      return;
   }

   _count_deadly_contacts++;
}

void GameContactListener::processPlayerFootSensorContactBegin(FixtureNode* fixture_node, b2Fixture* fixture)
{
   if (fixture->IsSensor())
   {
      return;
   }

   if (isEnemy(fixture_node))
   {
      return;
   }

   // store ground body in player
   if (fixture->GetType() == b2Shape::e_chain)
   {
      Player::getCurrent()->setGroundBody(fixture->GetBody());
   }

   _count_foot_contacts++;
}

void GameContactListener::processPlayerHeadSensorContactBegin(b2Fixture* fixture)
{
   if (fixture->IsSensor())
   {
      return;
   }

   _count_head_contacts++;
}

void GameContactListener::processPlayerLeftArmSensorContactBegin(b2Fixture* fixture)
{
   if (fixture->IsSensor())
   {
      return;
   }

   _count_arm_left_contacts++;
}

void GameContactListener::processPlayerRightArmSensorContactBegin(b2Fixture* fixture)
{
   if (fixture->IsSensor())
   {
      return;
   }

   _count_arm_right_contacts++;
}

void GameContactListener::processOneWayWallContactBegin(b2Contact* contact, b2Fixture* fixture)
{
   OneWayWall::instance().beginContact(contact, fixture);
}

void GameContactListener::processPlayerContactBegin()
{
   _count_player_contacts++;
}

void GameContactListener::processDeadlyContactBegin(FixtureNode* fixture_node)
{
   if (!isPlayer(fixture_node))
   {
      return;
   }

   _count_deadly_contacts++;
}

void GameContactListener::processEnemyContactBegin(FixtureNode* fixture_node_a, FixtureNode* fixture_node_b)
{
   if (!isPlayer(fixture_node_b))
   {
      return;
   }

   const auto damage = std::get<int32_t>(fixture_node_a->getProperty("damage"));
   fixture_node_a->collisionWithPlayer();
   Player::getCurrent()->damage(damage);
}

void GameContactListener::processBouncerContactBegin(FixtureNode* fixture_node)
{
   dynamic_cast<Bouncer*>(fixture_node)->activate();
}

void GameContactListener::processBubbleCubeContactBegin(b2Contact* contact, FixtureNode* bubble, FixtureNode* other)
{
   dynamic_cast<BubbleCube*>(bubble)->beginContact(contact, other);
}

void GameContactListener::processCollapsingPlatformContactBegin(b2Contact* contact, FixtureNode* platform, FixtureNode* other)
{
   dynamic_cast<CollapsingPlatform*>(platform)->beginContact(contact, other);
}

void GameContactListener::processBeginContact(
   b2Contact* contact,
   b2Fixture* contact_fixture_a,
   b2Fixture* contact_fixture_b,
   FixtureNode* fixture_node_a,
   FixtureNode* fixture_node_b,
   void* fixture_user_data_b
)
{
   switch (fixture_node_a->getType())
   {
      case ObjectTypeCrusher:
      {
         processCrusherContactBegin(fixture_node_b);
         break;
      }
      case ObjectTypePlayerFootSensor:
      {
         processPlayerFootSensorContactBegin(fixture_node_b, contact_fixture_b);
         break;
      }
      case ObjectTypePlayerHeadSensor:
      {
         processPlayerHeadSensorContactBegin(contact_fixture_b);
         break;
      }
      case ObjectTypePlayerLeftArmSensor:
      {
         processPlayerLeftArmSensorContactBegin(contact_fixture_b);
         break;
      }
      case ObjectTypePlayerRightArmSensor:
      {
         processPlayerRightArmSensorContactBegin(contact_fixture_b);
         break;
      }
      case ObjectTypeProjectile:
      {
         processProjectileContactBegin(fixture_node_a, fixture_node_b);
         break;
      }
      case ObjectTypeSolidOneWay:
      {
         processOneWayWallContactBegin(contact, contact_fixture_b);
         break;
      }
      case ObjectTypePlayer:
      {
         processPlayerContactBegin();
         break;
      }
      case ObjectTypeDeadly:
      {
         processDeadlyContactBegin(fixture_node_b);
         break;
      }
      case ObjectTypeMovingPlatform:
      {
         processMovingPlatformContactBegin(contact_fixture_a, fixture_user_data_b);
         break;
      }
      case ObjectTypeBouncer:
      {
         processBouncerContactBegin(fixture_node_a);
         break;
      }
      case ObjectTypeEnemy:
      {
         processEnemyContactBegin(fixture_node_a, fixture_node_b);
         break;
      }
      case ObjectTypeBubbleCube:
      {
         processBubbleCubeContactBegin(contact, fixture_node_a, fixture_node_b);
         break;
      }
      case ObjectTypeCollapsingPlatform:
      {
         processCollapsingPlatformContactBegin(contact, fixture_node_a, fixture_node_b);
         break;
      }
      case ObjectTypeDoor:
      case ObjectTypeConveyorBelt:
      case ObjectTypeMoveableBox:
      case ObjectTypeDeathBlock:
      case ObjectTypeSolid:
      case ObjectTypeInvalid:
      {
         break;
      }
   }
}

void GameContactListener::BeginContact(b2Contact* contact)
{
   auto fixture_user_data_a = contact->GetFixtureA()->GetUserData();
   auto fixture_user_data_b = contact->GetFixtureB()->GetUserData();
   auto contact_fixture_a = contact->GetFixtureA();
   auto contact_fixture_b = contact->GetFixtureB();

   FixtureNode* fixture_node_a = nullptr;
   FixtureNode* fixture_node_b = nullptr;

   if (fixture_user_data_a)
   {
      fixture_node_a = static_cast<FixtureNode*>(fixture_user_data_a);
   }

   if (fixture_user_data_b)
   {
      fixture_node_b = static_cast<FixtureNode*>(fixture_user_data_b);
   }

   if (fixture_user_data_a)
   {
      processBeginContact(contact, contact_fixture_a, contact_fixture_b, fixture_node_a, fixture_node_b, fixture_user_data_b);
   }

   if (fixture_user_data_b)
   {
      processBeginContact(contact, contact_fixture_b, contact_fixture_a, fixture_node_b, fixture_node_a, fixture_user_data_a);
   }
}

void GameContactListener::processCrusherContactEnd(FixtureNode* fixture_node)
{
   if (!isPlayer(fixture_node))
   {
      return;
   }

   _count_deadly_contacts--;
}

void GameContactListener::processPlayerFootSensorContactEnd(FixtureNode* fixture_node, b2Fixture* fixture)
{
   if (fixture->IsSensor())
   {
      return;
   }

   // contact with enemies is not taken into regard for foot sensor because that'd enable him to jump off enemies
   if (isEnemy(fixture_node))
   {
      return;
   }

   _count_foot_contacts--;
}

void GameContactListener::processPlayerHeadSensorContactEnd(auto contact_fixture_b)
{
   if (contact_fixture_b->IsSensor())
   {
      return;
   }

   _count_head_contacts--;
}

void GameContactListener::processPlayerLeftArmSensorContactEnd(b2Fixture* contact_fixture)
{
   if (contact_fixture->IsSensor())
   {
      return;
   }

   _count_arm_left_contacts--;
}

void GameContactListener::processPlayerRightArmSensorContactEnd(b2Fixture* contact_fixture)
{
   if (contact_fixture->IsSensor())
   {
      return;
   }

   _count_arm_right_contacts--;
}

void GameContactListener::processPlayerContactEnd()
{
   _count_player_contacts--;
}

void GameContactListener::processOneWayWallContactEnd(b2Contact* contact)
{
   OneWayWall::instance().endContact(contact);
}

void GameContactListener::processDeadlyContactEnd(FixtureNode* fixture_node)
{
   if (!isPlayer(fixture_node))
   {
      return;
   }

   _count_deadly_contacts--;
}

void GameContactListener::processMovingPlatformContactEnd()
{
   _count_moving_platform_contacts--;
}

void GameContactListener::processBubbleCubeContactEnd(FixtureNode* fixture_node_bubble, FixtureNode* fixture_node_other)
{
   dynamic_cast<BubbleCube*>(fixture_node_bubble)->endContact(fixture_node_other);
}

void GameContactListener::processCollapsingPlatformContactEnd(FixtureNode* fixture_node_platform, FixtureNode* fixture_node_other)
{
   dynamic_cast<CollapsingPlatform*>(fixture_node_platform)->endContact(fixture_node_other);
}

void GameContactListener::processEndContact(
   b2Contact* contact,
   FixtureNode* fixture_node_a,
   FixtureNode* fixture_node_b,
   b2Fixture* contact_fixture_b
)
{
   switch (fixture_node_a->getType())
   {
      case ObjectTypeCrusher:
      {
         processCrusherContactEnd(fixture_node_b);
         break;
      }
      case ObjectTypePlayerFootSensor:
      {
         processPlayerFootSensorContactEnd(fixture_node_b, contact_fixture_b);
         break;
      }
      case ObjectTypePlayerHeadSensor:
      {
         processPlayerHeadSensorContactEnd(contact_fixture_b);
         break;
      }
      case ObjectTypePlayerLeftArmSensor:
      {
         processPlayerLeftArmSensorContactEnd(contact_fixture_b);
         break;
      }
      case ObjectTypePlayerRightArmSensor:
      {
         processPlayerRightArmSensorContactEnd(contact_fixture_b);
         break;
      }
      case ObjectTypePlayer:
      {
         processPlayerContactEnd();
         break;
      }
      case ObjectTypeSolidOneWay:
      {
         processOneWayWallContactEnd(contact);
         break;
      }
      case ObjectTypeDeadly:
      {
         processDeadlyContactEnd(fixture_node_b);
         break;
      }
      case ObjectTypeMovingPlatform:
      {
         processMovingPlatformContactEnd();
         break;
      }
      case ObjectTypeBubbleCube:
      {
         processBubbleCubeContactEnd(fixture_node_a, fixture_node_b);
         break;
      }
      case ObjectTypeCollapsingPlatform:
      {
         processCollapsingPlatformContactEnd(fixture_node_a, fixture_node_b);
         break;
      }
      default:
      {
         break;
      }
   }
}

void GameContactListener::EndContact(b2Contact* contact)
{
   auto fixture_user_data_a = contact->GetFixtureA()->GetUserData();
   auto fixture_user_data_b = contact->GetFixtureB()->GetUserData();

   FixtureNode* fixture_node_a = nullptr;
   FixtureNode* fixture_node_b = nullptr;

   if (fixture_user_data_a)
   {
      fixture_node_a = static_cast<FixtureNode*>(fixture_user_data_a);
   }

   if (fixture_user_data_b)
   {
      fixture_node_b = static_cast<FixtureNode*>(fixture_user_data_b);
   }

   auto contact_fixture_a = contact->GetFixtureA();
   auto contact_fixture_b = contact->GetFixtureB();

   if (fixture_user_data_a)
   {
      processEndContact(contact, fixture_node_a, fixture_node_b, contact_fixture_b);
   }

   if (fixture_user_data_b)
   {
      processEndContact(contact, fixture_node_b, fixture_node_a, contact_fixture_a);
   }
}

void GameContactListener::PreSolve(b2Contact* contact, const b2Manifold* /*oldManifold*/)
{
   ConveyorBelt::processContact(contact);
}

void GameContactListener::processPostSolve(FixtureNode* node, float impulse)
{
   if (node->getType() == ObjectTypePlayer)
   {
      processPostSolveImpulse(impulse);
   }
   else if (node->getType() == ObjectTypeProjectile)
   {
      processPostSolveProjectile(node, impulse);
   }
}

void GameContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* contact_impulse)
{
   // normal impulse
   //
   //    The normal force is the force applied at a contact point to prevent the shapes from penetrating.
   //    For convenience, Box2D works with impulses. The normal impulse is just the normal force multiplied
   //    by the time step.
   //
   //
   // tangent impulse
   //
   //    The tangent force is generated at a contact point to simulate friction. For convenience,
   //    this is stored as an impulse.
   //

// #define DEBUG_IMPULSES
#ifdef DEBUG_IMPULSES
   auto normal_max = 0.0f;
   auto tangent_max = 0.0f;
   for (auto i = 0; i < contact_impulse->count; i++)
   {
      normal_max = std::max(normal_max, contact_impulse->normalImpulses[i]);
      tangent_max = std::max(tangent_max, contact_impulse->tangentImpulses[i]);
   }

   if (normal_max > 0.025f || tangent_max > 0.01f)
   {
      Log::Info() << "normal max: " << normal_max << " tangent max: " << tangent_max;
   }
#endif

   // check if the player hits something at a heigh speed or
   // if something hits the player at a nigh speed
   auto user_data_a = contact->GetFixtureA()->GetUserData();
   auto user_data_b = contact->GetFixtureB()->GetUserData();

   const auto impulse = contact_impulse->normalImpulses[0];

   if (user_data_a)
   {
      auto node_a = static_cast<FixtureNode*>(user_data_a);
      processPostSolve(node_a, impulse);
   }

   if (user_data_b)
   {
      auto node_b = static_cast<FixtureNode*>(user_data_b);
      processPostSolve(node_b, impulse);
   }
}

void GameContactListener::debug()
{
   std::cout << "head contacts: " << getPlayerHeadContactCount() << std::endl
             << "foot contacts: " << getPlayerFootContactCount() << std::endl
             << "deadly contacts: " << getDeadlyContactCount() << std::endl
             << "moving platform contacts: " << getMovingPlatformContactCount() << std::endl
             << "player contacts: " << getPlayerContactCount() << std::endl;
}

void GameContactListener::processPostSolveImpulse(float impulse)
{
   // filter just ordinary ground contact
   if (impulse < 0.03f)
   {
      return;
   }

   Player::getCurrent()->impulse(impulse);
}

void GameContactListener::processPostSolveProjectile(FixtureNode* node, float impulse)
{
   auto projectile = dynamic_cast<Projectile*>(node);

   if (!projectile->isSticky())
   {
      return;
   }

   if (projectile->hitSomething())
   {
      return;
   }

   projectile->setHitSomething(true);

   // have a delay between hit and sprite removal
   //
   // this is only needed for arrows, so could be generalised
   // Timer::add(
   //    std::chrono::milliseconds(1000),
   //    [projectile](){projectile->setScheduledForRemoval(true);},
   //    Timer::Type::Singleshot,
   //    Timer::Scope::UpdateIngame
   // );

   projectile->setScheduledForRemoval(true);

   if (impulse > 0.0003f)
   {
      // Log::Info() << "arrow hit with " << impulse;
      projectile->setScheduledForInactivity(true);
   }
}

int32_t GameContactListener::getPlayerArmRightContactCount() const
{
   return _count_arm_right_contacts;
}

bool GameContactListener::isPlayerSmashed() const
{
   return _smashed;
}

int32_t GameContactListener::getPlayerArmLeftContactCount() const
{
   return _count_arm_left_contacts;
}

int32_t GameContactListener::getPlayerHeadContactCount() const
{
   return _count_head_contacts;
}

void GameContactListener::reset()
{
   _count_head_contacts = 0;
   _count_foot_contacts = 0;
   _count_player_contacts = 0;
   _count_arm_left_contacts = 0;
   _count_arm_right_contacts = 0;
   _count_deadly_contacts = 0;
   _count_moving_platform_contacts = 0;
   _smashed = false;
   OneWayWall::instance().clear();
}

GameContactListener& GameContactListener::getInstance()
{
   static GameContactListener __instance;
   return __instance;
}

int32_t GameContactListener::getPlayerContactCount() const
{
   return _count_player_contacts;
}

int32_t GameContactListener::getMovingPlatformContactCount() const
{
   return _count_moving_platform_contacts;
}
