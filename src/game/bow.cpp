#include "bow.h"
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
// static constexpr auto drag_constant = 0.1f;
static constexpr auto scale = 0.1f;

const auto sprite_width = PIXELS_PER_TILE;
const auto sprite_height = PIXELS_PER_TILE;
const auto sprite_count = 5;
const auto sprites_per_row = 15;
const auto sprite_frame_time = 0.075f;
const auto sprite_start_frame = 10;

uint16_t categoryBits = CategoryEnemyCollideWith;                // I am a ...
uint16_t maskBitsStanding = CategoryBoundary | CategoryFriendly; // I collide with ...
int16_t groupIndex = 0;                                          // 0 is default
}


bool Arrow::_animation_initialised = false;


Arrow::Arrow()
{
   _rotating = true;
   _sticky = true;
   _weapon_type = WeaponType::Bow;

   if (!_animation_initialised)
   {
      _animation_initialised = true;

      auto texture = TexturePool::getInstance().get("data/weapons/arrow.png");

      std::vector<sf::Time> frame_times;
      for (auto i = 0u; i < sprite_count; i++)
      {
         frame_times.push_back(sf::seconds(sprite_frame_time));
      }

      sf::Vector2f origin(
         static_cast<float_t>(PIXELS_PER_TILE / 2),
         static_cast<float_t>(PIXELS_PER_TILE / 2)
      );

      _hit_animations.emplace(
         _weapon_type,
         ProjectileHitAnimation::FrameData{
            texture,
            origin,
            sprite_width,
            sprite_height,
            sprite_count,
            sprites_per_row,
            frame_times,
            sprite_start_frame
         }
      );
   }
}


void Arrow::updateTextureRect(const sf::Time& /*time*/)
{
   sf::Rect<int32_t> texture_rect;
   texture_rect.top    = 1 * PIXELS_PER_TILE;
   texture_rect.left   = 2 * PIXELS_PER_TILE;
   texture_rect.width  = PIXELS_PER_TILE;
   texture_rect.height = PIXELS_PER_TILE;

   _sprite.setTextureRect(texture_rect);
}


Bow::Bow()
{
   _fire_interval_ms = 1500;
}


Bow::~Bow()
{
   std::for_each(begin(_arrows), end(_arrows), [](auto ptr) {delete ptr;});
   _arrows.clear();
}


void Bow::load(b2World* world)
{
   auto arrow = _loaded_arrow = new Arrow();
   arrow->setSprite(_projectile_reference_sprite);

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


void Bow::loadTextures()
{
   // the shape is only defined here to align the texture on it
   _shape = std::make_unique<b2PolygonShape>();
   dynamic_cast<b2PolygonShape*>(_shape.get())->SetAsBox(
      (fabs(arrow_tail) + fabs(arrow_tip)) * scale,
      arrow_width * scale
   );

   _texture_path = "data/weapons/arrow.png";

   _projectile_reference_texture = TexturePool::getInstance().get(_texture_path);

   _projectile_reference_sprite.setTexture(*_projectile_reference_texture);
   _projectile_reference_sprite.setOrigin(
      static_cast<float_t>(PIXELS_PER_TILE / 2),
      static_cast<float_t>(PIXELS_PER_TILE / 2)
   );
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
      arrow->updateTextureRect(time);

      if (!arrow->getBody()->IsActive())
      {
         continue;
      }

      auto arrow_body = arrow->getBody();

      auto arrow_velocity = arrow_body->GetLinearVelocity();
      /*const auto arrlow_velocity_length =*/ arrow_velocity.Normalize();

      arrow->setRotation(atan2(arrow_velocity.y, arrow_velocity.x));

      // we don't really need realistic arrow drag in this game
      //
      // const auto arrow_tail_position = arrow_body->GetWorldPoint(b2Vec2(arrow_tail, 0.0f));
      // auto arrow_pointing_direction = arrow_body->GetWorldVector(b2Vec2(1.0f, 0.0f));
      // arrow_pointing_direction.Normalize();
      //
      // const auto dot = b2Dot(arrow_velocity, arrow_pointing_direction);
      //
      // const auto draw_force_magnitude =
      //    (1 - fabs(dot)) * arrlow_velocity_length * arrlow_velocity_length * drag_constant * arrow_body->GetMass();
      //
      // arrow_body->ApplyForce(
      //    draw_force_magnitude * -arrow_velocity,
      //    arrow_tail_position,
      //    false
      // );
   }
}


