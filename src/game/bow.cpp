#include "bow.h"

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
static constexpr auto launch_speed = 200.0f;
static constexpr auto arrow_tail = -1.4f;
static constexpr auto arrow_tip = 0.6f;
static constexpr auto arrow_width = 0.1f;
static constexpr auto drag_constant = 0.1f;
static constexpr auto scale = 0.1f;

uint16_t categoryBits = CategoryEnemyCollideWith;                // I am a ...
uint16_t maskBitsStanding = CategoryBoundary | CategoryFriendly; // I collide with ...
int16_t groupIndex = 0;                                          // 0 is default
}


Bow::Bow()
{
   _fire_interval_ms = 1500;
}


void Bow::load(b2World* world)
{
   auto arrow = _loaded_arrow = new Arrow();

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
   b2Vec2 posCopy = pos;
   posCopy.y -= 0.15f;

   // let the bow always point up a bit
   b2Vec2 dirCopy = dir;
   dirCopy.y -= 0.01f;

   // for now just let the arrow point in the fire direction
   // rotation can be done later
   if (dir.x < 0.0)
   {
      _projectile_texture_rect.top = 0 * PIXELS_PER_TILE;
   }
   else
   {
      _projectile_texture_rect.top = 1 * PIXELS_PER_TILE;
   }

   _projectile_texture_rect.left   = 2 * PIXELS_PER_TILE;
   _projectile_texture_rect.width  = PIXELS_PER_TILE;
   _projectile_texture_rect.height = PIXELS_PER_TILE;
   _projectile_sprite.setTextureRect(_projectile_texture_rect);

   // the bow workflow could be split up into
   // 1) aim
   // 2) pull
   // 3) release
   // Right now it's just firing into walking direction.
   load(world.get());

   _arrows.push_back(_loaded_arrow);

   // store projectile so it gets drawn
   _projectiles.push_back(_loaded_arrow);

   const auto angle = atan2(dirCopy.y, dirCopy.x);
   const auto velocity = _launcher_body->GetWorldVector(launch_speed * dirCopy);

   _loaded_arrow->getBody()->SetAwake(true);
   _loaded_arrow->getBody()->SetGravityScale(1.0f);
   _loaded_arrow->getBody()->SetAngularVelocity(0.0f);
   _loaded_arrow->getBody()->SetTransform(posCopy, angle);
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


void Bow::loadTextures()
{
   // the shape is only defined here to align the texture on it
   _shape = std::make_unique<b2PolygonShape>();
   dynamic_cast<b2PolygonShape*>(_shape.get())->SetAsBox(
      (fabs(arrow_tail) + fabs(arrow_tip)) * scale,
      arrow_width * scale
   );

   _texture_path = "data/weapons/arrow.png";

   if (!_projectile_texture.loadFromFile(_texture_path.string()))
   {
      std::cout << "Bow::loadTextures(): couldn't load texture " << _texture_path.string() << std::endl;
   }
   else
   {
      _projectile_sprite.setTexture(_projectile_texture);

      _projectile_sprite.setOrigin(
         static_cast<float_t>(PIXELS_PER_TILE / 2),
         static_cast<float_t>(PIXELS_PER_TILE / 2)
      );
   }
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

      const auto arrow_tail_position = arrow_body->GetWorldPoint(b2Vec2(arrow_tail, 0.0f));
      auto arrow_pointing_direction = arrow_body->GetWorldVector(b2Vec2(1.0f, 0.0f));
      arrow_pointing_direction.Normalize();

      auto arrow_velocity = arrow_body->GetLinearVelocity();
      const auto arrlow_velocity_length = arrow_velocity.Normalize();

      const auto dot = b2Dot(arrow_velocity, arrow_pointing_direction);
      arrow->_angle = acos(dot);

      const auto draw_force_magnitude =
         (1 - fabs(dot)) * arrlow_velocity_length * arrlow_velocity_length * drag_constant * arrow_body->GetMass();

      arrow_body->ApplyForce(
         draw_force_magnitude * -arrow_velocity,
         arrow_tail_position,
         false
      );
   }
}

