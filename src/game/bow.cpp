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
static constexpr auto launch_speed = 15.0f;
static constexpr auto arrow_tail = -1.4f;
static constexpr auto arrow_tip = 0.6f;
static constexpr auto arrow_width = 0.1f;
static constexpr auto drag_constant = 0.1f;
static constexpr auto scale = 0.1f;
}


void Bow::load(b2World* world)
{
   _loaded_arrow = new Arrow();

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

   auto loaded_arrow_body = world->CreateBody(&bodyDef);
   loaded_arrow_body->CreateFixture( &fixtureDef );
   loaded_arrow_body->SetAngularDamping(3);
   loaded_arrow_body->SetGravityScale(0.0f);

   _loaded_arrow->setBody(loaded_arrow_body);
}


void Bow::fireNow(
   const std::shared_ptr<b2World>& world,
   const b2Vec2& pos,
   const b2Vec2& dir
)
{
   load(world.get());

   const auto angle = atan2(dir.y, dir.x);
   const auto velocity = _launcher_body->GetWorldVector(b2Vec2(launch_speed, 0.0f));

   _loaded_arrow->getBody()->SetAwake(true);
   _loaded_arrow->getBody()->SetGravityScale(1.0f);
   _loaded_arrow->getBody()->SetAngularVelocity(0.0f);
   _loaded_arrow->getBody()->SetTransform(pos, angle);
   _loaded_arrow->getBody()->SetLinearVelocity(velocity);

   _arrows.push_back(_loaded_arrow);
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


b2Body* Bow::getLauncherBody() const
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
   _loaded_arrow->getBody()->SetTransform(start_position, _launcher_body->GetAngle());

   // apply drag force to arrows
   for (auto i = 0u; i < _arrows.size(); i++)
   {
      auto arrow_body = _arrows[i]->getBody();

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


