// base
#include "gamecontactlistener.h"

// game
#include "audio.h"
#include "projectile.h"
#include "constants.h"
#include "fixturenode.h"
#include "framework/tools/timer.h"
#include "luanode.h"
#include "mechanisms/bouncer.h"
#include "mechanisms/bubblecube.h"
#include "mechanisms/conveyorbelt.h"
#include "mechanisms/movingplatform.h"
#include "player/player.h"

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
   if (obj == nullptr)
   {
      return false;
   }

   auto p = dynamic_cast<Player*>(obj->getParent());

   if (p == nullptr)
   {
      return false;
   }

   return true;
}


void GameContactListener::processOneSidedWalls(b2Contact* contact, b2Fixture* player_fixture, b2Fixture* platform_fixture)
{
   // decide whether an incoming contact to the platform should be disabled or not

   // if the head bounces against the one-sided wall, disable the contact
   // until there is no more contact with the head (EndContact)
   if (player_fixture != nullptr && (static_cast<FixtureNode*>(player_fixture->GetUserData()))->hasFlag("head"))
   {
      contact->SetEnabled(false);
   }

   if (!platform_fixture)
   {
      return;
   }

   // if moving down, the contact should be solid
   if (player_fixture->GetBody()->GetLinearVelocity().y > 0.0f)
   {
      return;
   }

   // not all points are moving down towards the platform, the contact should not be solid
   contact->SetEnabled(false);
}


void GameContactListener::BeginContact(b2Contact* contact)
{
   auto fixture_user_data_a = contact->GetFixtureA()->GetUserData();
   auto fixture_user_data_b = contact->GetFixtureB()->GetUserData();

   b2Fixture* platform_fixture = nullptr;
   b2Fixture* player_fixture = nullptr;

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
      switch (fixture_node_a->getType())
      {
         case ObjectTypeCrusher:
         {
            if (isPlayer(fixture_node_b))
            {
               _count_deadly_contacts++;
            }
            break;
         }
         case ObjectTypePlayerFootSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               // store ground body in player
               if (contact->GetFixtureB()->GetType() == b2Shape::e_chain)
               {
                  Player::getCurrent()->setGroundBody(contact->GetFixtureB()->GetBody());
               }
               _count_foot_contacts++;
            }
            break;
         }
         case ObjectTypePlayerHeadSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               _count_head_contacts++;
            }
            break;
         }
         case ObjectTypePlayerLeftArmSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               _count_arm_left_contacts++;
            }
            break;
         }
         case ObjectTypePlayerRightArmSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               _count_arm_right_contacts++;
            }
            break;
         }
         case ObjectTypeProjectile:
         {
            auto damage = std::get<int32_t>(fixture_node_a->getProperty("damage"));

            if (isPlayer(fixture_node_b))
            {
               Player::getCurrent()->damage(damage);
            }
            else if (fixture_node_b && fixture_node_b->getType() == ObjectTypeEnemy)
            {
               auto p = dynamic_cast<LuaNode*>(fixture_node_b->getParent());
               if (p != nullptr)
               {
                  p->luaHit(damage);
               }
            }

            auto projectile = dynamic_cast<Projectile*>(fixture_node_a);

            // if it's an arrow, let postsolve handle it. if the impulse is not
            // hard enough, the arrow should just fall on the ground
            if (!projectile->isSticky())
            {
               projectile->setScheduledForRemoval(true);
            }

            break;
         }
         case ObjectTypeSolidOneSided:
         {
            platform_fixture = contact->GetFixtureA();
            player_fixture = contact->GetFixtureB();
            break;
         }
         case ObjectTypePlayer:
         {
            _count_player_contacts++;
            break;
         }
         case ObjectTypeDeadly:
         {
            if (isPlayer(fixture_node_b))
            {
               _count_deadly_contacts++;
            }
            break;
         }
         case ObjectTypeMovingPlatform:
         {
            // check if platform smashes the player
            auto fixture_node_b = static_cast<FixtureNode*>(fixture_user_data_b);
            if (fixture_node_b && fixture_node_b->getType() == ObjectType::ObjectTypePlayerHeadSensor)
            {
               if (Player::getCurrent()->isOnGround())
               {
                  _smashed = true;
               }
            }

            auto platform_body = contact->GetFixtureA()->GetBody();
            Player::getCurrent()->setPlatformBody(platform_body);

            _count_moving_platform_contacts++;
            break;
         }
         case ObjectTypeBouncer:
         {
            dynamic_cast<Bouncer*>(fixture_node_a)->activate();
            break;
         }
         case ObjectTypeEnemy:
         {
            if (isPlayer(fixture_node_b))
            {
               // printf("collision with enemy\n");
               auto damage = std::get<int32_t>(fixture_node_a->getProperty("damage"));
               fixture_node_a->collisionWithPlayer();
               Player::getCurrent()->damage(damage);
               break;
            }
            break;
         }
         case ObjectTypeDoor:
            break;
         case ObjectTypeConveyorBelt:
            break;
         case ObjectTypeMoveableBox:
            break;
         case ObjectTypeDeathBlock:
            break;
         case ObjectTypeSolid:
            break;
         case ObjectTypeBubbleCube:
         {
            dynamic_cast<BubbleCube*>(fixture_node_a)->beginContact();
            break;
         }
      }
   }

   if (fixture_user_data_b)
   {
      FixtureNode* fixture_node_b = static_cast<FixtureNode*>(fixture_user_data_b);

      switch (fixture_node_b->getType())
      {
         case ObjectTypeCrusher:
         {
            if (isPlayer(fixture_node_a))
            {
               _count_deadly_contacts++;
            }
            break;
         }
         case ObjectTypePlayerFootSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               // store ground body in player
               if (contact->GetFixtureA()->GetType() == b2Shape::e_chain)
               {
                  Player::getCurrent()->setGroundBody(contact->GetFixtureA()->GetBody());
               }

               _count_foot_contacts++;
            }
            break;
         }
         case ObjectTypePlayerHeadSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               _count_head_contacts++;
            }
            break;
         }
         case ObjectTypePlayerLeftArmSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               _count_arm_left_contacts++;
            }
            break;
         }
         case ObjectTypePlayerRightArmSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               _count_arm_right_contacts++;
            }
            break;
         }
         case ObjectTypeProjectile:
         {
            auto damage = std::get<int32_t>(fixture_node_b->getProperty("damage"));

            if (isPlayer(fixture_node_a))
            {
               Player::getCurrent()->damage(damage);
            }
            else if (fixture_node_a && fixture_node_a->getType() == ObjectTypeEnemy)
            {
               auto p = dynamic_cast<LuaNode*>(fixture_node_a->getParent());
               if (p != nullptr)
               {
                  p->luaHit(damage);
               }
            }

            auto projectile = dynamic_cast<Projectile*>(fixture_node_b);

            // if it's an arrow, let postsolve handle it. if the impulse is not
            // hard enough, the arrow should just fall on the ground
            if (!projectile->isSticky())
            {
               projectile->setScheduledForRemoval(true);
            }

            break;
         }
         case ObjectTypeSolidOneSided:
         {
            platform_fixture = contact->GetFixtureB();
            player_fixture = contact->GetFixtureA();
            break;
         }
         case ObjectTypePlayer:
         {
            _count_player_contacts++;
            break;
         }
         case ObjectTypeDeadly:
         {
            if (isPlayer(fixture_node_a))
            {
               _count_deadly_contacts++;
            }
            break;
         }
         case ObjectTypeMovingPlatform:
         {
            // check if platform smashes the player
            auto fixtureNodeA = static_cast<FixtureNode*>(fixture_user_data_a);
            if (fixtureNodeA && fixtureNodeA->getType() == ObjectType::ObjectTypePlayerHeadSensor)
            {
               if (Player::getCurrent()->isOnGround())
               {
                  _smashed = true;
               }
            }

            auto platformBody = contact->GetFixtureB()->GetBody();
            Player::getCurrent()->setPlatformBody(platformBody);

            _count_moving_platform_contacts++;
            break;
         }
         case ObjectTypeBouncer:
         {
            dynamic_cast<Bouncer*>(fixture_node_b)->activate();
            break;
         }
         case ObjectTypeEnemy:
         {
            if (isPlayer(fixture_node_a))
            {
               // printf("collision with enemy\n");
               auto damage = std::get<int32_t>(fixture_node_b->getProperty("damage"));
               fixture_node_b->collisionWithPlayer();
               Player::getCurrent()->damage(damage);
               break;
            }
            break;
         }
         case ObjectTypeDoor:
            break;
         case ObjectTypeConveyorBelt:
            break;
         case ObjectTypeMoveableBox:
            break;
         case ObjectTypeDeathBlock:
            break;
         case ObjectTypeSolid:
            break;
         case ObjectTypeBubbleCube:
         {
            dynamic_cast<BubbleCube*>(fixture_node_b)->beginContact();
            break;
         }
      }
   }

   // handle one sided walls
   processOneSidedWalls(contact, player_fixture, platform_fixture);
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

   if (fixture_user_data_a)
   {
      switch (fixture_node_a->getType())
      {
         case ObjectTypeCrusher:
         {
            if (isPlayer(fixture_node_b))
            {
               _count_deadly_contacts--;
            }
            break;
         }
         case ObjectTypePlayerFootSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               _count_foot_contacts--;
            }
            break;
         }
         case ObjectTypePlayerHeadSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               _count_head_contacts--;
            }
            break;
         }
         case ObjectTypePlayerLeftArmSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               _count_arm_left_contacts--;
            }
            break;
         }
         case ObjectTypePlayerRightArmSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               _count_arm_right_contacts--;
            }
            break;
         }
         case ObjectTypePlayer:
         {
            _count_player_contacts--;
            break;
         }
         case ObjectTypeSolidOneSided:
         {
            // reset the default state of the contact
            contact->SetEnabled(true);
            break;
         }
         case ObjectTypeDeadly:
         {
            if (isPlayer(fixture_node_b))
            {
               _count_deadly_contacts--;
            }
            break;
         }
         case ObjectTypeMovingPlatform:
         {
            _count_moving_platform_contacts--;
            break;
         }
         case ObjectTypeBubbleCube:
         {
            dynamic_cast<BubbleCube*>(fixture_node_a)->endContact();
            break;
         }
         default:
         {
            break;
         }
      }
   }

   if (fixture_user_data_b)
   {
      switch (fixture_node_b->getType())
      {
         case ObjectTypeCrusher:
         {
            if (isPlayer(fixture_node_a))
            {
               _count_deadly_contacts--;
            }
            break;
         }
         case ObjectTypePlayerFootSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               _count_foot_contacts--;
            }
            break;
         }
         case ObjectTypePlayerHeadSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               _count_head_contacts--;
            }
            break;
         }
         case ObjectTypePlayerLeftArmSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               _count_arm_left_contacts--;
            }
            break;
         }
         case ObjectTypePlayerRightArmSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               _count_arm_right_contacts--;
            }
            break;
         }
         case ObjectTypePlayer:
         {
            _count_player_contacts--;
            break;
         }
         case ObjectTypeSolidOneSided:
         {
            // reset the default state of the contact
            contact->SetEnabled(true);
            break;
         }
         case ObjectTypeDeadly:
         {
            if (isPlayer(fixture_node_a))
            {
               _count_deadly_contacts--;
            }
            break;
         }
         case ObjectTypeMovingPlatform:
         {
            _count_moving_platform_contacts--;
            break;
         }
         case ObjectTypeBubbleCube:
         {
            dynamic_cast<BubbleCube*>(fixture_node_b)->endContact();
            break;
         }
         default:
         {
            break;
         }
      }
   }

   // debug();
   // Log::Info() << "left arm: " << mNumArmLeftContacts << " " << "right arm: " << mNumArmRightContacts;
}


void GameContactListener::PreSolve(b2Contact* contact, const b2Manifold* /*oldManifold*/)
{
   ConveyorBelt::processContact(contact);
}


void GameContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse *contactImpulse)
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
   //
   // for debugging
   //
   // auto normalMax = 0.0f;
   // auto tangentMax = 0.0f;
   // for (auto i = 0; i < contactImpulse->count; i++)
   // {
   //    normalMax = std::max(normalMax, contactImpulse->normalImpulses[i]);
   //    tangentMax = std::max(tangentMax, contactImpulse->tangentImpulses[i]);
   // }
   //
   // if (normalMax > 0.025f || tangentMax > 0.01f)
   // {
   //    Log::Info() << "normal max: " << normalMax << " tangent max: " << tangentMax;
   // }

   // check if the player hits something at a heigh speed or
   // if something hits the player at a nigh speed
   auto user_data_a = contact->GetFixtureA()->GetUserData();
   auto user_data_b = contact->GetFixtureB()->GetUserData();

   auto impulse = contactImpulse->normalImpulses[0];

   if (user_data_a)
   {
      auto node_a = static_cast<FixtureNode*>(user_data_a);

      if (node_a->getType() == ObjectTypePlayer)
      {
         processImpulse(impulse);
      }
      else if (node_a->getType() == ObjectTypeProjectile)
      {
         auto projectile = dynamic_cast<Projectile*>(node_a);

         if (projectile->isSticky())
         {
            if (projectile->hitSomething())
            {
               return;
            }

            projectile->setHitSomething(true);

            // this is only needed for arrows, so could be generalised
            Timer::add(
               std::chrono::milliseconds(1000),
               [projectile](){projectile->setScheduledForRemoval(true);},
               Timer::Type::Singleshot,
               Timer::Scope::UpdateIngame
            );

            if (impulse > 0.0003f)
            {
               // Log::Info() << "arrow hit with " << impulse;
               projectile->setScheduledForInactivity(true);
            }
         }
      }
   }

   if (user_data_b)
   {
      auto node_b = static_cast<FixtureNode*>(user_data_b);

      if (node_b->getType() == ObjectTypePlayer)
      {
         processImpulse(impulse);
      }
      else if (node_b->getType() == ObjectTypeProjectile)
      {
         auto projectile = dynamic_cast<Projectile*>(node_b);

         if (projectile->isSticky())
         {
            if (projectile->hitSomething())
            {
               return;
            }

            projectile->setHitSomething(true);

            // this is only needed for arrows, so could be generalised
            Timer::add(
               std::chrono::milliseconds(1000),
               [projectile](){projectile->setScheduledForRemoval(true);},
               Timer::Type::Singleshot,
               Timer::Scope::UpdateIngame
            );

            if (impulse > 0.0003f)
            {
               // Log::Info() << "arrow hit with " << impulse;
               projectile->setScheduledForInactivity(true);
            }
         }
      }
   }
}


void GameContactListener::debug()
{
   std::cout
      << "head contacts: " << getPlayerHeadContactCount() << std::endl
      << "foot contacts: " << getPlayerFootContactCount() << std::endl
      << "deadly contacts: " << getDeadlyContactCount() << std::endl
      << "moving platform contacts: " << getMovingPlatformContactCount() << std::endl
      << "player contacts: " << getPlayerContactCount() << std::endl
   ;
}


void GameContactListener::processImpulse(float impulse)
{
   // filter just ordinary ground contact
   if (impulse < 0.03f)
   {
      return;
   }

   Player::getCurrent()->impulse(impulse);
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


