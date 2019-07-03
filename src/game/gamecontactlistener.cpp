// base
#include "gamecontactlistener.h"

// game
#include "audio.h"
#include "bouncer.h"
#include "bullet.h"
#include "constants.h"
#include "conveyorbelt.h"
#include "fixturenode.h"
#include "player.h"

#include <iostream>


// http://www.iforce2d.net/b2dtut/collision-anatomy
//
// TODO: pass collision normal to bullet detonation
//       so animation can be aligned to detonation angle.

GameContactListener* GameContactListener::sInstance = nullptr;


GameContactListener::GameContactListener()
 : b2ContactListener(),
   mNumFootContacts(0),
   mNumPlayerContacts(0),
   mNumDeadlyContacts(0),
   mNumMovingPlatformContacts(0)
{
  sInstance = this;
}


int GameContactListener::getNumFootContacts() const
{
   return mNumFootContacts;
}


int GameContactListener::getDeadlyContacts() const
{
   return mNumDeadlyContacts;
}


int GameContactListener::getPlayerId(FixtureNode* obj)
{
   if (obj == nullptr)
   {
      return -1;
   }

   auto p = dynamic_cast<Player*>(obj->getParent());
   if (p == nullptr)
   {
      return -1;
   }

   return p->getId();
}


void GameContactListener::processOneSidedWalls(b2Contact* contact, b2Fixture* playerFixture, b2Fixture* platformFixture)
{
   if (platformFixture != nullptr)
   {
      int numPoints = contact->GetManifold()->pointCount;
      b2WorldManifold worldManifold;
      contact->GetWorldManifold( &worldManifold );

      b2Body* platformBody = platformFixture->GetBody();
      b2Body* playerBody = playerFixture->GetBody();

      bool disable = false;

      // check if any of the contact points are moving from the bottom into platform
      for (int i = 0; i < numPoints; i++)
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
            // point is moving into platform, leave contact solid and exit
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
         case ObjectTypePlayerFootSensor:
         {
            mNumFootContacts++;
            break;
         }
         case ObjectTypeBullet:
         {
            if (getPlayerId(fixtureNodeB) != -1)
            {
               auto damage = std::get<int32_t>(fixtureNodeA->getProperty("damage"));
               Player::getPlayer(0)->damage(damage);
            }
            dynamic_cast<Bullet*>(fixtureNodeA)->setScheduledForRemoval(true);
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
            if (getPlayerId(fixtureNodeB) != -1)
               mNumDeadlyContacts++;
            break;
         }
         case ObjectTypeMovingPlatform:
         {
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
            if (getPlayerId(fixtureNodeB) != -1)
            {
               printf("collision with enemy\n");
               auto damage = std::get<int32_t>(fixtureNodeA->getProperty("damage"));
               Player::getPlayer(0)->damage(damage);
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
      }
   }

   if (fixtureUserDataB)
   {
      FixtureNode* fixtureNodeB = static_cast<FixtureNode*>(fixtureUserDataB);

      switch (fixtureNodeB->getType())
      {
         case ObjectTypePlayerFootSensor:
         {
            mNumFootContacts++;
            break;
         }
         case ObjectTypeBullet:
         {
            if (getPlayerId(fixtureNodeA) != -1)
            {
               auto damage = std::get<int32_t>(fixtureNodeB->getProperty("damage"));
               Player::getPlayer(0)->damage(damage);
            }
            dynamic_cast<Bullet*>(fixtureNodeB)->setScheduledForRemoval(true);
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
            if (getPlayerId(fixtureNodeA) != -1)
               mNumDeadlyContacts++;
            break;
         }
         case ObjectTypeMovingPlatform:
         {
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
            if (getPlayerId(fixtureNodeA) != -1)
            {
               printf("collision with enemy\n");
               auto damage = std::get<int32_t>(fixtureNodeB->getProperty("damage"));
               Player::getPlayer(0)->damage(damage);
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

   if (fixtureUserDataA)
   {
      auto fixtureNode = static_cast<FixtureNode*>(fixtureUserDataA);

      switch (fixtureNode->getType())
      {
         case ObjectTypePlayerFootSensor:
            mNumFootContacts--;
            break;
         case ObjectTypePlayer:
            mNumPlayerContacts--;
            break;
         case ObjectTypeOneSidedWall:
            contact->SetEnabled(true);
            break;
         case ObjectTypeDeadly:
            mNumDeadlyContacts--;
            break;
         case ObjectTypeMovingPlatform:
            mNumMovingPlatformContacts--;
            break;
         default:
            break;
      }
   }

   if (fixtureUserDataB)
   {
      auto fixtureNode = static_cast<FixtureNode*>(fixtureUserDataB);

      switch (fixtureNode->getType())
      {
         case ObjectTypePlayerFootSensor:
            mNumFootContacts--;
            break;
         case ObjectTypePlayer:
            mNumPlayerContacts--;
            break;
         case ObjectTypeOneSidedWall:
            contact->SetEnabled(true);
            break;
         case ObjectTypeDeadly:
            mNumDeadlyContacts--;
            break;
         case ObjectTypeMovingPlatform:
            mNumMovingPlatformContacts--;
            break;
         default:
            break;
      }
   }

   // printf("end: %d\n", mNumPlayerContacts);
}


void GameContactListener::PreSolve(b2Contact *contact, const b2Manifold* /*oldManifold*/)
{
  ConveyorBelt::processContact(contact);
}


void GameContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse *contactImpulse)
{
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


void GameContactListener::processImpulse(float impulse)
{
   // filter just ordinary ground contact
   if (impulse < 0.03f)
   {
      return;
   }

   Player::getPlayer(0)->impulse(impulse);
}


void GameContactListener::reset()
{
   mNumFootContacts = 0;
   mNumPlayerContacts = 0;
   mNumDeadlyContacts = 0;
   mNumMovingPlatformContacts = 0;
}


GameContactListener *GameContactListener::getInstance()
{
  if (!sInstance)
  {
    new GameContactListener();
  }

  return sInstance;
}


int GameContactListener::getNumPlayerContacts() const
{
   return mNumPlayerContacts;
}


int GameContactListener::getNumMovingPlatformContacts() const
{
   return mNumMovingPlatformContacts;
}


