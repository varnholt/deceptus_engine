#include "bow.h"

// http://www.iforce2d.net/b2dtut/sticky-projectiles


// |         |         |         |         |
// |         |         |         |         |
// |         |   ######x         |         |
// |      #######      |#####    |         |
// +-----x---+---------+-----x---+---------+ y = 0
// |      #######      |#####    |         |
// |         |   ######x         |         |
// |         |         |         |         |
//                   x = 0


namespace
{
static constexpr auto launch_speed = 10.0f;
static constexpr auto arrow_tail = -1.4f;
static constexpr auto arrow_tip = 0.6f;
static constexpr auto arrow_width = 0.1f;
static constexpr auto drag_constant = 0.1f;
static constexpr auto scale = 0.1f;
}


void Bow::load(b2World* world)
{
   b2BodyDef bodyDef;
   bodyDef.type = b2_dynamicBody;
   bodyDef.position.Set(0, 5);

   b2PolygonShape polygonShape;
   b2Vec2 vertices[4];
   vertices[0].Set(arrow_tail * scale,  0.0f               );
   vertices[1].Set( 0.0,               -arrow_width * scale);
   vertices[2].Set(arrow_tip * scale,   0.0f               );
   vertices[3].Set( 0.0,                arrow_width * scale);
   polygonShape.Set(vertices, 4);

   b2FixtureDef fixtureDef;
   fixtureDef.shape = &polygonShape;
   fixtureDef.density = 1.0f;

   _loaded_arrow_body = world->CreateBody(&bodyDef);
   _loaded_arrow_body->CreateFixture( &fixtureDef );
   _loaded_arrow_body->SetAngularDamping(3);

   // until fired
   _loaded_arrow_body->SetGravityScale(0.0f);
}


void Bow::fireNow(
   const std::shared_ptr<b2World>& world,
   const b2Vec2& pos,
   const b2Vec2& dir
)
{
   load(world.get());

   _loaded_arrow_body->SetAwake(true);
   _loaded_arrow_body->SetGravityScale(1.0f);
   _loaded_arrow_body->SetAngularVelocity(0.0f);

   _loaded_arrow_body->SetTransform(
      pos,
      atan2(dir.y, dir.x)
   );

   _loaded_arrow_body->SetLinearVelocity(
      _launcher_body->GetWorldVector(b2Vec2(launch_speed, 0.0f))
   );

   _arrow_bodies.push_back(_loaded_arrow_body);
}


void Bow::postSolve(b2Contact* contact, const b2ContactImpulse* impulse)
{
   b2Fixture* fixtureA = contact->GetFixtureA();
   b2Fixture* fixtureB = contact->GetFixtureB();

   if (impulse->normalImpulses[0] > 0.5f) // targetInfoA->hardness
   {
      ArrowCollision collision;
      collision._target = fixtureA->GetBody();
      collision._arrow = fixtureB->GetBody();

      _arrow_collisions.push_back(collision);
   }
   else if (impulse->normalImpulses[0] > 0.5f) // targetInfoB->hardness
   {
      ArrowCollision collision;
      collision._target = fixtureB->GetBody();
      collision._arrow = fixtureA->GetBody();

      _arrow_collisions.push_back(collision);
   }
}


b2Body* Bow::getLauncheBody() const
{
   return _launcher_body;
}


void Bow::setLauncherBody(b2Body* launcher_body)
{
   _launcher_body = launcher_body;
}


void Bow::update()
{
   // position the loaded arrow
   const auto start_position = _launcher_body->GetWorldPoint(b2Vec2(3.5f, 0));
   _loaded_arrow_body->SetTransform(start_position, _launcher_body->GetAngle());

   // apply drag force to arrows
   for (auto i = 0u; i < _arrow_bodies.size(); i++)
   {
      auto arrow_body = _arrow_bodies[i];

      const auto arrow_tail_position = arrow_body->GetWorldPoint(b2Vec2(arrow_tail, 0.0f));
      const auto arrow_pointing_direction = arrow_body->GetWorldVector(b2Vec2(1.0f, 0.0f));

      auto arrow_velocity = arrow_body->GetLinearVelocity();
      const auto arrow_dir = arrow_velocity.Normalize();
      const auto dot = b2Dot(arrow_velocity, arrow_pointing_direction);

      const auto draw_force_magnitude = (1 - fabs(dot)) * arrow_dir * arrow_dir * drag_constant * arrow_body->GetMass();

      arrow_body->ApplyForce(
         draw_force_magnitude * -arrow_velocity,
         arrow_tail_position,
         false
      );
   }

   // keep the arrow for a bit, then discard it
   _arrow_collisions.clear();
   // world->DestroyBody(arrow_body);
}


