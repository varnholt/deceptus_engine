#include "moveablebox.h"

#include "audio.h"
#include "constants.h"
#include "fixturenode.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "texturepool.h"

#include <iostream>

//--------------------------------------------------------------------------------------------------
MoveableBox::MoveableBox(GameNode* node) : GameNode(node)
{
   setClassName(typeid(MoveableBox).name());

   _texture = TexturePool::getInstance().get("data/level-malte/tilesets/crypts.png");
   _sprite.setTexture(*_texture.get());
}

//--------------------------------------------------------------------------------------------------
void MoveableBox::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   color.draw(_sprite);
}

//--------------------------------------------------------------------------------------------------
void MoveableBox::update(const sf::Time& /*dt*/)
{
   const auto x = _body->GetPosition().x * PPM;
   const auto y = _body->GetPosition().y * PPM;
   _sprite.setPosition(x, y - 24);

   // if the thing is moving, start playing a scratching sound
   if (fabs(_body->GetLinearVelocity().x) > 0.01)
   {
      if (!_pushing_sample.has_value())
      {
         _pushing_sample = Audio::getInstance().playSample({"mechanism_moveable_object_01.wav", 1.0, true});
      }
   }
   else
   {
      if (_pushing_sample.has_value())
      {
         Audio::getInstance().stopSample(_pushing_sample.value());
         _pushing_sample.reset();
      }
   }
}

//--------------------------------------------------------------------------------------------------
std::optional<sf::FloatRect> MoveableBox::getBoundingBoxPx()
{
   return _sprite.getGlobalBounds();
}

// box: pos: 5160 x 1056 size: 48 x 48
// box: pos: 5376 x 1080 size: 24 x 24

/*

   +----+
   | __ |
   |//\\|
   +----+
   |####|
   |####|
   +----+

   +----+----+
   | ___|___ |
   |////|\\\\|
   +----+----+
   |####|####|
   |####|####|
   +----+----+
   |####|####|
   |####|####|
   +----+----+


*/

//--------------------------------------------------------------------------------------------------
void MoveableBox::setup(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);

   _size.x = data._tmx_object->_width_px;
   _size.y = data._tmx_object->_height_px;

   _sprite.setPosition(data._tmx_object->_x_px, data._tmx_object->_y_px - 24);

   if (data._tmx_object->_properties)
   {
      auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         auto z_index = static_cast<uint32_t>(z_it->second->_value_int.value());
         setZ(z_index);
      }
   }

   switch (static_cast<int32_t>(_size.x))
   {
      case 24:
      {
         _sprite.setTextureRect(sf::IntRect(1392, 0, 24, 2 * 24));
         break;
      }

      case 48:
      {
         _sprite.setTextureRect(sf::IntRect(1296, 24, 2 * 24, 3 * 24));
         break;
      }

      default:
      {
         break;
      }
   }

   setupBody(data._world);
   setupTransform();
}

//-----------------------------------------------------------------------------
void MoveableBox::setupTransform()
{
   auto x = _sprite.getPosition().x / PPM;
   auto y = _sprite.getPosition().y / PPM;
   _body->SetTransform(b2Vec2(x, y), 0);
}

//--------------------------------------------------------------------------------------------------
void MoveableBox::setupBody(const std::shared_ptr<b2World>& world)
{
   b2PolygonShape polygon_shape;
   auto size_x = _size.x / PPM;
   auto size_y = _size.y / PPM;

   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(0, 0);
   vertices[1] = b2Vec2(0, size_y);
   vertices[2] = b2Vec2(size_x, size_y);
   vertices[3] = b2Vec2(size_x, 0);

   polygon_shape.Set(vertices, 4);

   b2BodyDef body_def;
   body_def.type = b2_dynamicBody;
   _body = world->CreateBody(&body_def);
   // mBody->SetGravityScale(0.0f);

   setupTransform();

   auto fixture = _body->CreateFixture(&polygon_shape, 0);
   auto object_data = new FixtureNode(this);
   object_data->setType(ObjectTypeMoveableBox);
   fixture->SetUserData(static_cast<void*>(object_data));
}
