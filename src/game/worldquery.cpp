#include "worldquery.h"


std::vector<b2Fixture*> WorldQuery::queryFixtures(const std::shared_ptr<b2World>& world, const b2AABB& aabb)
{
   FixtureQueryCallback query_callback;
   world->QueryAABB(&query_callback, aabb);
   return query_callback._fixtures;
}


bool WorldQuery::FixtureQueryCallback::ReportFixture(b2Fixture* fixture)
{
   _fixtures.push_back(fixture);
   return true;
}


std::vector<b2Body*> WorldQuery::queryBodies(const std::shared_ptr<b2World>& world, const b2AABB& aabb)
{
   BodyQueryCallback query_callback;
   world->QueryAABB(&query_callback, aabb);
   return query_callback._bodies;
}


bool WorldQuery::BodyQueryCallback::ReportFixture(b2Fixture* fixture)
{
   _bodies.push_back(fixture->GetBody());
   return true;
}
