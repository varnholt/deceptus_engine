#pragma once


// box2d
#include "Box2D/Box2D.h"

#include <vector>

class FixtureNode;

class GameContactListener : public b2ContactListener
{

public:

   GameContactListener();

   int32_t getNumHeadContacts() const;
   int32_t getNumFootContacts() const;
   int32_t getDeadlyContacts() const;
   int32_t getNumMovingPlatformContacts() const;
   int32_t getNumPlayerContacts() const;

   void BeginContact(b2Contact *contact) override;
   void EndContact(b2Contact *contact) override;

   void PreSolve(b2Contact *contact, const b2Manifold *oldManifold) override;
   void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override;

   void reset();

   static GameContactListener* getInstance();


protected:

   bool isPlayer(FixtureNode* obj) const;


private:

   void processOneSidedWalls(b2Contact* contact, b2Fixture* playerFixture, b2Fixture* platformFixture);
   void processImpulse(float impulse);

   void processBeginContact(
      b2Contact* contact,
      b2Fixture* playerFixture,
      b2Fixture* otherThingFixture,
      FixtureNode* playerFixtureNode,
      FixtureNode* otherThingFixtureNode
   );


private:

   int32_t mNumFootContacts = 0;
   int32_t mNumHeadContacts = 0;
   int32_t mNumPlayerContacts = 0;
   int32_t mNumDeadlyContacts = 0;
   int32_t mNumMovingPlatformContacts = 0;
   int32_t mBouncerCycles = 5;

   static GameContactListener* sInstance;
};

