#pragma once


// box2d
#include "Box2D/Box2D.h"

#include <vector>

class FixtureNode;

class GameContactListener : public b2ContactListener
{

public:

   GameContactListener();

   int getNumFootContacts() const;
   int getDeadlyContacts() const;
   int getNumMovingPlatformContacts() const;
   int getNumPlayerContacts() const;

   void BeginContact(b2Contact *contact) override;
   void EndContact(b2Contact *contact) override;

   void PreSolve(b2Contact *contact, const b2Manifold *oldManifold) override;
   void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override;

   void reset();

   static GameContactListener* getInstance();


protected:

   int getPlayerId(FixtureNode *obj);


private:

   void processOneSidedWalls(b2Contact* contact, b2Fixture* playerFixture, b2Fixture* platformFixture);
   void processPlayerDamage(const b2ContactImpulse *contactImpulse, b2Contact *contact);
   void processImpulse(float impulse);


private:

   int mNumFootContacts = 0;
   int mNumPlayerContacts = 0;
   int mNumDeadlyContacts = 0;
   int mNumMovingPlatformContacts = 0;

   static GameContactListener* sInstance;
};

