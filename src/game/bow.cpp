#include "bow.h"

#include "arrow.h"
#include "framework/tools/globalclock.h"
#include "texturepool.h"

#include <iostream>

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

// those two need to be configurable (dynamic objects need higher launch speeds apparently)
static constexpr auto launch_speed = 5.0f;
static constexpr auto arrow_gravity_scale = 0.1f;

static constexpr auto arrow_tail = -1.4f;
static constexpr auto arrow_tip = 0.6f;
static constexpr auto arrow_width = 0.1f;
static constexpr auto scale = 0.1f;

static constexpr auto start_frame = 15;
static constexpr auto frames_per_row = 15;
static constexpr auto animation_frame_count = 4;

uint16_t categoryBits = CategoryEnemyCollideWith;                // I am a ...
uint16_t maskBitsStanding = CategoryBoundary | CategoryFriendly; // I collide with ...
int16_t groupIndex = 0;                                          // 0 is default
}


Bow::Bow()
{
   _fire_interval_ms = 1500;

   // the shape is only defined here to align the texture on it
   _shape = std::make_unique<b2PolygonShape>();
   dynamic_cast<b2PolygonShape*>(_shape.get())->SetAsBox(
      (fabs(arrow_tail) + fabs(arrow_tip)) * scale,
      arrow_width * scale
   );

   // create reference animation from frame data
   AnimationFrameData frame_data(
      TexturePool::getInstance().get("data/weapons/arrow.png"),
      {static_cast<float_t>(PIXELS_PER_TILE / 2), static_cast<float_t>(PIXELS_PER_TILE / 2)},
      PIXELS_PER_TILE,
      PIXELS_PER_TILE,
      animation_frame_count,
      frames_per_row,
      {sf::seconds(0.075f), sf::seconds(0.075f), sf::seconds(0.075f), sf::seconds(0.075f)},
      start_frame
   );

   setProjectileAnimation(frame_data);
}


Bow::~Bow()
{
   std::for_each(begin(_arrows), end(_arrows), [](auto ptr) {delete ptr;});
   _arrows.clear();
}


void Bow::load(b2World* world)
{
   auto arrow = _loaded_arrow = new Arrow();

   arrow->setAnimation(_projectile_reference_animation._animation);
   arrow->_start_time = GlobalClock::getInstance()->getElapsedTimeInMs();

   _loaded_arrow->addDestroyedCallback([this, arrow](){
      _arrows.erase(std::remove(_arrows.begin(), _arrows.end(), arrow), _arrows.end());
   });

   _loaded_arrow->addDestroyedCallback([this, arrow](){
      _projectiles.erase(std::remove(_projectiles.begin(), _projectiles.end(), arrow), _projectiles.end());
   });

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
   fixtureDef.filter.groupIndex   = groupIndex;
   fixtureDef.filter.maskBits     = maskBitsStanding;
   fixtureDef.filter.categoryBits = categoryBits;

   auto loaded_arrow_body = world->CreateBody(&bodyDef);
   auto fixture = loaded_arrow_body->CreateFixture(&fixtureDef);
   loaded_arrow_body->SetAngularDamping(3);
   loaded_arrow_body->SetGravityScale(0.0f);
   fixture->SetUserData(_loaded_arrow);

   _loaded_arrow->setBody(loaded_arrow_body);
}


void Bow::fireNow(
   const std::shared_ptr<b2World>& world,
   const b2Vec2& pos,
   const b2Vec2& dir
)
{
   // the bow workflow could be split up into
   // 1) aim
   // 2) pull
   // 3) release
   // Right now it's just firing into walking direction.
   load(world.get());

   _arrows.push_back(_loaded_arrow);

   // store projectile so it gets drawn
   _projectiles.push_back(_loaded_arrow);

   const auto angle = atan2(dir.y, dir.x);
   const auto velocity = _launcher_body->GetWorldVector(launch_speed * dir);

   _loaded_arrow->getBody()->SetAwake(true);
   _loaded_arrow->getBody()->SetGravityScale(arrow_gravity_scale);
   _loaded_arrow->getBody()->SetAngularVelocity(0.0f);
   _loaded_arrow->getBody()->SetTransform(pos, angle);
   _loaded_arrow->getBody()->SetLinearVelocity(velocity);
   _loaded_arrow->setProperty("damage", _damage);
   _loaded_arrow = nullptr;
}


b2Body* Bow::getLauncherBody() const
{
   return _launcher_body;
}


void Bow::setLauncherBody(b2Body* launcher_body)
{
   _launcher_body = launcher_body;
}


void Bow::update(const sf::Time& time)
{
   Weapon::update(time);

   // position the loaded arrow
   if (_loaded_arrow)
   {
      // position the arrow next to the object carrying it (half a meter away)
      const auto start_position = _launcher_body->GetWorldPoint(b2Vec2(0.5f, 0));
      _loaded_arrow->getBody()->SetTransform(start_position, _launcher_body->GetAngle());
   }

   // apply drag force to arrows
   for (auto& arrow : _arrows)
   {
      if (!arrow->getBody()->IsActive())
      {
         continue;
      }

      auto arrow_body = arrow->getBody();
      auto arrow_velocity = arrow_body->GetLinearVelocity();
      arrow_velocity.Normalize();
      arrow->setRotation(atan2(arrow_velocity.y, arrow_velocity.x));
   }
}


