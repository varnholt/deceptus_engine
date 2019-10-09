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


void GameContactListener::processBeginContact(
    b2Contact* contact,
    b2Fixture* playerFixture,
    b2Fixture* otherThingFixture,
    FixtureNode* playerFixtureNode,
    FixtureNode* otherThingFixtureNode
)
{
    switch (otherThingFixtureNode->getType())
    {
       case ObjectTypePlayerFootSensor:
       {
          if (!otherThingFixture->IsSensor())
          {
             mNumFootContacts++;
          }
          break;
       }
       case ObjectTypePlayerHeadSensor:
       {
          if (!otherThingFixture->IsSensor())
          {
             mNumHeadContacts++;
          }
          break;
       }
       case ObjectTypeBullet:
       {
          if (isPlayer(playerFixtureNode))
          {
             auto damage = std::get<int32_t>(otherThingFixtureNode->getProperty("damage"));
             Player::getCurrent()->damage(damage);
          }
          dynamic_cast<Bullet*>(otherThingFixtureNode)->setScheduledForRemoval(true);
          break;
       }
       case ObjectTypeOneSidedWall:
       {
          processOneSidedWalls(contact, playerFixture, otherThingFixture);
          break;
       }
       case ObjectTypePlayer:
       {
          mNumPlayerContacts++;
          break;
       }
       case ObjectTypeDeadly:
       {
          if (isPlayer(playerFixtureNode))
          {
             mNumDeadlyContacts++;
          }
          break;
       }
       case ObjectTypeMovingPlatform:
       {
          mNumMovingPlatformContacts++;
          break;
       }
       case ObjectTypeBouncer:
       {
          dynamic_cast<Bouncer*>(otherThingFixtureNode)->activate();
          break;
       }
       case ObjectTypeEnemy:
       {
          if (isPlayer(playerFixtureNode))
          {
             printf("collision with enemy\n");
             auto damage = std::get<int32_t>(otherThingFixtureNode->getProperty("damage"));
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
    }
}


void GameContactListener::BeginContact(b2Contact* contact)
{
   auto fixtureUserDataA = contact->GetFixtureA()->GetUserData();
   auto fixtureUserDataB = contact->GetFixtureB()->GetUserData();

   b2Fixture* playerFixture = nullptr;

   if (fixtureUserDataA)
   {
      auto otherThingFixtureNode = static_cast<FixtureNode*>(fixtureUserDataA);
      auto otherThingFixture = contact->GetFixtureB();
      auto playerFixtureNode = static_cast<FixtureNode*>(fixtureUserDataB);
      playerFixture = contact->GetFixtureA();

      processBeginContact(contact, playerFixture, otherThingFixture, playerFixtureNode, otherThingFixtureNode);
   }

   if (fixtureUserDataB)
   {
      auto otherThingFixtureNode = static_cast<FixtureNode*>(fixtureUserDataB);
      auto otherThingFixture = contact->GetFixtureA();
      auto playerFixtureNode = static_cast<FixtureNode*>(fixtureUserDataA);
      playerFixture = contact->GetFixtureB();

      processBeginContact(contact, playerFixture, otherThingFixture, playerFixtureNode, otherThingFixtureNode);
   }

// that might have to be called before processOneSidedWalls or as part of it to make it work
//
//   if (playerFixture != nullptr && (static_cast<FixtureNode*>(playerFixture->GetUserData()))->hasFlag("head"))
//   {
//      contact->SetEnabled(false);
//   }
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

   Player::getCurrent()->impulse(impulse);
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


