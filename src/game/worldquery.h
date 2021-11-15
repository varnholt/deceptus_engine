#pragma once

#include <Box2D/Box2D.h>

#include <vector>


namespace WorldQuery
{

class FixtureQueryCallback : public b2QueryCallback
{
   public:

      bool ReportFixture(b2Fixture* fixture);
      std::vector<b2Fixture*> _fixtures;
};


class BodyQueryCallback : public b2QueryCallback
{
   public:

      bool ReportFixture(b2Fixture* fixture);
      std::vector<b2Body*> _bodies;
};


std::vector<b2Fixture*> queryFixtures(const std::shared_ptr<b2World>& world, const b2AABB& aabb);
std::vector<b2Body*> queryBodies(const std::shared_ptr<b2World>& world, const b2AABB& aabb);

}

