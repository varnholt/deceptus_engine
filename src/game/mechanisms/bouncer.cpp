#include "bouncer.h"

#include "framework/tools/globalclock.h"
#include "game/audio/audio.h"
#include "game/fixturenode.h"
#include "game/player/player.h"
#include "game/texturepool.h"

#include <iostream>

const auto SPRITE_WIDTH = 24;
const auto SPRITE_HEIGHT = 24;

//
//        +--------------------------------------------------+ <-
//        |                                                  | sensor surface
//        +--------------------------------------------------+ <-
//        |                                                  |
//        |                                                  |
//        |                                                  |
//        |                                                  |
//        |                                                  |
//        |                                                  |
//        +--------------------------------------------------+
//

b2Body* Bouncer::getBody() const
{
   return _body;
}

Bouncer::Bouncer(GameNode* parent, const GameDeserializeData& data) : FixtureNode(parent)
{
   setClassName(typeid(Bouncer).name());
   setObjectId(data._tmx_object->_name);
   setZ(data._tmx_object_group->_z_index);

   const auto x = data._tmx_object->_x_px;
   const auto y = data._tmx_object->_y_px;
   const auto width = data._tmx_object->_width_px;
   const auto height = data._tmx_object->_height_px;

   _rect.left = x;
   _rect.top = y;
   _rect.width = width;
   _rect.height = height;

   setType(ObjectTypeBouncer);
   _activation_time = GlobalClock::getInstance().getElapsedTime();

   _position_b2d = b2Vec2(x * MPP, y * MPP);
   _position_sfml.x = x;
   _position_sfml.y = y + height;

   b2BodyDef bodyDef;
   bodyDef.type = b2_staticBody;
   bodyDef.position = _position_b2d;

   _body = data._world->CreateBody(&bodyDef);

   auto half_physics_width = width * MPP * 0.5f;
   auto half_physics_height = height * MPP * 0.5f;

   // create fixture for physical boundaries of the bouncer object
   _shape_bounds.SetAsBox(half_physics_width, half_physics_height, b2Vec2(half_physics_width, half_physics_height), 0.0f);

   b2FixtureDef boundaryFixtureDef;
   boundaryFixtureDef.shape = &_shape_bounds;
   boundaryFixtureDef.density = 1.0f;
   boundaryFixtureDef.isSensor = false;

   _body->CreateFixture(&boundaryFixtureDef);

   // create fixture for the sensor behavior, collision notification
   _shape_bounds.SetAsBox(half_physics_width, half_physics_height, b2Vec2(half_physics_width, half_physics_height), 0.0f);

   b2FixtureDef sensor_fixture_def;
   sensor_fixture_def.shape = &_shape_bounds;
   sensor_fixture_def.isSensor = true;

   auto fixture = _body->CreateFixture(&sensor_fixture_def);
   fixture->SetUserData(static_cast<void*>(this));

   // load texture
   _texture = TexturePool::getInstance().get(data._base_path / "tilesets" / "bumper.png");
   _sprite.setTexture(*_texture);
   _sprite.setPosition(_position_sfml - sf::Vector2f(0.0f, static_cast<float>(SPRITE_HEIGHT)));
}

void Bouncer::preload()
{
   Audio::getInstance().addSample("mechanism_bouncer.wav");
}

void Bouncer::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   color.draw(_sprite);
}

void Bouncer::updatePlayerAtBouncer()
{
   auto player = Player::getCurrent();
   auto rect = player->getPixelRectFloat();
   rect.height *= 3;

   _player_at_bouncer = rect.intersects(_rect);
}

void Bouncer::update(const sf::Time& /*dt*/)
{
   updatePlayerAtBouncer();

   const auto now = GlobalClock::getInstance().getElapsedTime();
   const auto delta_ms = (now - _activation_time).asMilliseconds();

   auto step = static_cast<int32_t>(delta_ms * 0.02f);
   if (step > 9)
   {
      step = 0;
   }

   _sprite.setTextureRect(sf::IntRect(step * SPRITE_WIDTH, 0, SPRITE_WIDTH, SPRITE_HEIGHT));
}

std::optional<sf::FloatRect> Bouncer::getBoundingBoxPx()
{
   return _rect;
}

bool Bouncer::isPlayerAtBouncer()
{
   return _player_at_bouncer;
}

void Bouncer::activate()
{
   const auto now = GlobalClock::getInstance().getElapsedTime();
   const auto delta_s = (now - _activation_time).asSeconds();

   // block spamming
   if (delta_s < 0.3f)
   {
      return;
   }

   _activation_time = now;

   constexpr auto force_value = 0.6f;

   b2Vec2 force(0.0f, 0.0f);
   switch (_alignment)
   {
      case Alignment::PointsUp:
         force = b2Vec2{0.0f, -force_value};
         break;
      case Alignment::PointsDown:
         force = b2Vec2{0.0f, force_value};
         break;
      case Alignment::PointsLeft:
         force = b2Vec2{-force_value, 0};
         break;
      case Alignment::PointsRight:
         force = b2Vec2{force_value, 0};
         break;
      case Alignment::PointsNowhere:
         break;
   }

   auto body = Player::getCurrent()->getBody();

   // it's pretty important to reset the body's y velocity
   const auto& velocity = body->GetLinearVelocity();
   body->SetLinearVelocity(b2Vec2(velocity.x, 0.0f));

   // aaaaand.. up!
   const auto& pos = body->GetWorldCenter();
   body->ApplyLinearImpulse(force, pos, true);
   Audio::getInstance().playSample({"mechanism_bouncer.wav"});
}
