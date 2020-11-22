// base
#include "gamecontactlistener.h"

// game
#include "audio.h"
#include "projectile.h"
#include "constants.h"
#include "fixturenode.h"
#include "luanode.h"
#include "mechanisms/bouncer.h"
#include "mechanisms/conveyorbelt.h"
#include "mechanisms/movingplatform.h"
#include "player/player.h"

#include <iostream>


// http://www.iforce2d.net/b2dtut/collision-anatomy
//
// TODO: pass collision normal to projectile detonation
//       so animation can be aligned to detonation angle.

GameContactListener* GameContactListener::sInstance = nullptr;


GameContactListener::GameContactListener()
 : b2ContactListener()
{
  sInstance = this;
}


int32_t GameContactListener::getNumFootContacts() const
{
   return mNumFootContacts;
}


int32_t GameContactListener::getDeadlyContacts() const
{
   return mNumDeadlyContacts;
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


void GameContactListener::processOneSidedWalls(b2Contact* contact, b2Fixture* playerFixture, b2Fixture* platformFixture)
{
   if (playerFixture != nullptr && (static_cast<FixtureNode*>(playerFixture->GetUserData()))->hasFlag("head"))
   {
      contact->SetEnabled(false);
   }

   if (platformFixture != nullptr)
   {
      int32_t numPoints = contact->GetManifold()->pointCount;
      b2WorldManifold worldManifold;
      contact->GetWorldManifold( &worldManifold );

      b2Body* platformBody = platformFixture->GetBody();
      b2Body* playerBody = playerFixture->GetBody();

      bool disable = false;

      // check if any of the contact points are moving from the bottom into platform
      for (int32_t i = 0; i < numPoints; i++)
      {
          b2Vec2 velocityPlatform = platformBody->GetLinearVelocityFromWorldPoint(worldManifold.points[i]);
          b2Vec2 velocityPlayer   = playerBody->GetLinearVelocityFromWorldPoint(worldManifold.points[i]);
          b2Vec2 velocityDiff     = platformBody->GetLocalVector(velocityPlayer - velocityPlatform);

         /*
            [#############] platform
                  ^
                  |
                 (*) player

         */

         // if moving down faster than 1m/s, handle as before
         if (velocityDiff.y < -1.0f)
         {
            // point32_t is moving into platform, leave contact solid and exit
            disable = true;
            break;
         }
      }

      // no points are moving into platform, contact should not be solid
      if (disable)
      {
         contact->SetEnabled(false);
      }
   }
}


void GameContactListener::BeginContact(b2Contact* contact)
{
   auto fixtureUserDataA = contact->GetFixtureA()->GetUserData();
   auto fixtureUserDataB = contact->GetFixtureB()->GetUserData();

   b2Fixture* platformFixture = nullptr;
   b2Fixture* playerFixture = nullptr;

   FixtureNode* fixtureNodeA = nullptr;
   FixtureNode* fixtureNodeB = nullptr;

   if (fixtureUserDataA)
   {
      fixtureNodeA = static_cast<FixtureNode*>(fixtureUserDataA);
   }

   if (fixtureUserDataB)
   {
      fixtureNodeB = static_cast<FixtureNode*>(fixtureUserDataB);
   }

   if (fixtureUserDataA)
   {
      switch (fixtureNodeA->getType())
      {
         case ObjectTypeCrusher:
         {
            if (isPlayer(fixtureNodeB))
            {
               mNumDeadlyContacts++;
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
               mNumFootContacts++;
            }
            break;
         }
         case ObjectTypePlayerHeadSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               mNumHeadContacts++;
            }
            break;
         }
         case ObjectTypePlayerLeftArmSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               mNumArmLeftContacts++;
            }
            break;
         }
         case ObjectTypePlayerRightArmSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               mNumArmRightContacts++;
            }
            break;
         }
         case ObjectTypeProjectile:
         {
            auto damage = std::get<int32_t>(fixtureNodeA->getProperty("damage"));

            if (isPlayer(fixtureNodeB))
            {
               Player::getCurrent()->damage(damage);
            }
            else if (fixtureNodeB && fixtureNodeB->getType() == ObjectTypeEnemy)
            {
               auto p = dynamic_cast<LuaNode*>(fixtureNodeB->getParent());
               if (p != nullptr)
               {
                  p->luaHit(damage);
               }
            }

            dynamic_cast<Projectile*>(fixtureNodeA)->setScheduledForRemoval(true);
            break;
         }
         case ObjectTypeOneSidedWall:
         {
            platformFixture = contact->GetFixtureA();
            playerFixture = contact->GetFixtureB();
            break;
         }
         case ObjectTypePlayer:
         {
            mNumPlayerContacts++;
            break;
         }
         case ObjectTypeDeadly:
         {
            if (isPlayer(fixtureNodeB))
            {
               mNumDeadlyContacts++;
            }
            break;
         }
         case ObjectTypeMovingPlatform:
         {
            auto platformBody = contact->GetFixtureA()->GetBody();
            Player::getCurrent()->setPlatformBody(platformBody);

            mNumMovingPlatformContacts++;
            break;
         }
         case ObjectTypeBouncer:
         {
            dynamic_cast<Bouncer*>(fixtureNodeA)->activate();
            break;
         }
         case ObjectTypeEnemy:
         {
            if (isPlayer(fixtureNodeB))
            {
               // printf("collision with enemy\n");
               auto damage = std::get<int32_t>(fixtureNodeA->getProperty("damage"));
               fixtureNodeA->collisionWithPlayer();
               Player::getCurrent()->damage(damage);
               break;
            }
            break;
         }
         case ObjectTypeDoor:
            break;
         case ObjectTypeConveyorBelt:
            break;
         case ObjectTypeJumpPlatform:
            break;
        case ObjectTypeMoveableBox:
           break;
        case ObjectTypeDeathBlock:
           break;
        case ObjectTypeWall:
           break;
      }
   }

   if (fixtureUserDataB)
   {
      FixtureNode* fixtureNodeB = static_cast<FixtureNode*>(fixtureUserDataB);

      switch (fixtureNodeB->getType())
      {
         case ObjectTypeCrusher:
         {
            if (isPlayer(fixtureNodeA))
            {
               mNumDeadlyContacts++;
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

               mNumFootContacts++;
            }
            break;
         }
         case ObjectTypePlayerHeadSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               mNumHeadContacts++;
            }
            break;
         }
         case ObjectTypePlayerLeftArmSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               mNumArmLeftContacts++;
            }
            break;
         }
         case ObjectTypePlayerRightArmSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               mNumArmRightContacts++;
            }
            break;
         }
         case ObjectTypeProjectile:
         {
            auto damage = std::get<int32_t>(fixtureNodeB->getProperty("damage"));

            if (isPlayer(fixtureNodeA))
            {
               Player::getCurrent()->damage(damage);
            }
            else if (fixtureNodeA && fixtureNodeA->getType() == ObjectTypeEnemy)
            {
               auto p = dynamic_cast<LuaNode*>(fixtureNodeA->getParent());
               if (p != nullptr)
               {
                  p->luaHit(damage);
               }
            }

            dynamic_cast<Projectile*>(fixtureNodeB)->setScheduledForRemoval(true);
            break;
         }
         case ObjectTypeOneSidedWall:
         {
            platformFixture = contact->GetFixtureB();
            playerFixture = contact->GetFixtureA();
            break;
         }
         case ObjectTypePlayer:
         {
            mNumPlayerContacts++;
            break;
         }
         case ObjectTypeDeadly:
         {
            if (isPlayer(fixtureNodeA))
            {
               mNumDeadlyContacts++;
            }
            break;
         }
         case ObjectTypeMovingPlatform:
         {
            auto platformBody = contact->GetFixtureB()->GetBody();
            Player::getCurrent()->setPlatformBody(platformBody);

            mNumMovingPlatformContacts++;
            break;
         }
         case ObjectTypeBouncer:
         {
            dynamic_cast<Bouncer*>(fixtureNodeB)->activate();
            break;
         }
         case ObjectTypeEnemy:
         {
            if (isPlayer(fixtureNodeA))
            {
               // printf("collision with enemy\n");
               auto damage = std::get<int32_t>(fixtureNodeB->getProperty("damage"));
               fixtureNodeB->collisionWithPlayer();
               Player::getCurrent()->damage(damage);
               break;
            }
            break;
         }
         case ObjectTypeDoor:
            break;
         case ObjectTypeConveyorBelt:
            break;
         case ObjectTypeJumpPlatform:
            break;
         case ObjectTypeMoveableBox:
            break;
         case ObjectTypeDeathBlock:
            break;
         case ObjectTypeWall:
            break;
      }
   }

   if (playerFixture != nullptr && ( static_cast<FixtureNode*>(playerFixture->GetUserData()))->hasFlag("head") )
   {
      contact->SetEnabled(false);
   }

   // handle one sided walls
   processOneSidedWalls(contact, playerFixture, platformFixture);
}


void GameContactListener::EndContact(b2Contact* contact)
{
   auto fixtureUserDataA = contact->GetFixtureA()->GetUserData();
   auto fixtureUserDataB = contact->GetFixtureB()->GetUserData();

   FixtureNode* fixtureNodeA = nullptr;
   FixtureNode* fixtureNodeB = nullptr;

   if (fixtureUserDataA)
   {
      fixtureNodeA = static_cast<FixtureNode*>(fixtureUserDataA);
   }

   if (fixtureUserDataB)
   {
      fixtureNodeB = static_cast<FixtureNode*>(fixtureUserDataB);
   }

   if (fixtureUserDataA)
   {
      auto fixtureNodeA = static_cast<FixtureNode*>(fixtureUserDataA);

      switch (fixtureNodeA->getType())
      {
         case ObjectTypeCrusher:
         {
            if (isPlayer(fixtureNodeB))
            {
               mNumDeadlyContacts--;
            }
            break;
         }
         case ObjectTypePlayerFootSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               mNumFootContacts--;
            }
            break;
         }
         case ObjectTypePlayerHeadSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               mNumHeadContacts--;
            }
            break;
         }
         case ObjectTypePlayerLeftArmSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               mNumArmLeftContacts--;
            }
            break;
         }
         case ObjectTypePlayerRightArmSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               mNumArmRightContacts--;
            }
            break;
         }
         case ObjectTypePlayer:
         {
            mNumPlayerContacts--;
            break;
         }
         case ObjectTypeOneSidedWall:
         {
            contact->SetEnabled(true);
            break;
         }
         case ObjectTypeDeadly:
         {
            if (isPlayer(fixtureNodeB))
            {
               mNumDeadlyContacts--;
            }
            break;
         }
         case ObjectTypeMovingPlatform:
         {
            mNumMovingPlatformContacts--;
            break;
         }
         default:
         {
            break;
         }
      }
   }

   if (fixtureUserDataB)
   {
      auto fixtureNode = static_cast<FixtureNode*>(fixtureUserDataB);

      switch (fixtureNode->getType())
      {
         case ObjectTypeCrusher:
         {
            if (isPlayer(fixtureNodeA))
            {
               mNumDeadlyContacts--;
            }
            break;
         }
         case ObjectTypePlayerFootSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               mNumFootContacts--;
            }
            break;
         }
         case ObjectTypePlayerHeadSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               mNumHeadContacts--;
            }
            break;
         }
         case ObjectTypePlayerLeftArmSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               mNumArmLeftContacts--;
            }
            break;
         }
         case ObjectTypePlayerRightArmSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               mNumArmRightContacts--;
            }
            break;
         }
         case ObjectTypePlayer:
         {
            mNumPlayerContacts--;
            break;
         }
         case ObjectTypeOneSidedWall:
         {
            contact->SetEnabled(true);
            break;
         }
         case ObjectTypeDeadly:
         {
            if (isPlayer(fixtureNodeA))
            {
               mNumDeadlyContacts--;
            }
            break;
         }
         case ObjectTypeMovingPlatform:
         {
            mNumMovingPlatformContacts--;
            break;
         }
         default:
         {
            break;
         }
      }
   }

   // debug();
   // std::cout << "left arm: " << mNumArmLeftContacts << " " << "right arm: " << mNumArmRightContacts << std::endl;
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
   //    std::cout << "normal max: " << normalMax << " tangent max: " << tangentMax << std::endl;
   // }

   // check if the player hits something at a heigh speed or
   // if something hits the player at a nigh speed
   auto userDataA = contact->GetFixtureA()->GetUserData();
   auto userDataB = contact->GetFixtureB()->GetUserData();

   auto impulse = contactImpulse->normalImpulses[0];

   if (userDataA)
   {
      auto nodeA = static_cast<FixtureNode*>(userDataA);

      if (nodeA->getType() == ObjectTypePlayer)
      {
         processImpulse(impulse);
      }
   }

   if (userDataB)
   {
      auto nodeB = static_cast<FixtureNode*>(userDataB);

      if (nodeB->getType() == ObjectTypePlayer)
      {
         processImpulse(impulse);
      }
   }
}


void GameContactListener::debug()
{
   std::cout
      << "head contacts: " << getNumHeadContacts() << std::endl
      << "foot contacts: " << getNumFootContacts() << std::endl
      << "deadly contacts: " << getDeadlyContacts() << std::endl
      << "moving platform contacts: " << getNumMovingPlatformContacts() << std::endl
      << "player contacts: " << getNumPlayerContacts() << std::endl
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

int32_t GameContactListener::getNumArmRightContacts() const
{
   return mNumArmRightContacts;
}

int32_t GameContactListener::getNumArmLeftContacts() const
{
   return mNumArmLeftContacts;
}


int32_t GameContactListener::getNumHeadContacts() const
{
   return mNumHeadContacts;
}


void GameContactListener::reset()
{
   mNumHeadContacts = 0;
   mNumFootContacts = 0;
   mNumPlayerContacts = 0;
   mNumArmLeftContacts = 0;
   mNumArmRightContacts = 0;
   mNumDeadlyContacts = 0;
   mNumMovingPlatformContacts = 0;
}


GameContactListener* GameContactListener::getInstance()
{
  if (!sInstance)
  {
    new GameContactListener();
  }

  return sInstance;
}


int32_t GameContactListener::getNumPlayerContacts() const
{
   return mNumPlayerContacts;
}


int32_t GameContactListener::getNumMovingPlatformContacts() const
{
   return mNumMovingPlatformContacts;
}


