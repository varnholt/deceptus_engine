#include "rope.h"

#include "framework/math/sfmlmath.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/io/texturepool.h"
#include "game/player/player.h"

#include <array>
#include <iostream>

int32_t Rope::_instance_counter = 0;

Rope::Rope(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Rope).name());

   setZ(16);

   // chain element setup
   _rope_element_shape.SetAsBox(0.0125f, 0.0125f);
   _rope_element_fixture_def.shape = &_rope_element_shape;
   _rope_element_fixture_def.density = 20.0f;
   _rope_element_fixture_def.friction = 0.2f;

   _instance_counter++;
   _push_time_s = static_cast<float>(_instance_counter);
}

void Rope::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   std::optional<b2Vec2> q1_prev;
   std::optional<b2Vec2> q4_prev;

   std::vector<sf::Vertex> strip;

   for (auto i = 0u; i < _chain_elements.size() - 1; i++)
   {
      auto* c1 = _chain_elements[i];
      auto* c2 = _chain_elements[i + 1];

      const auto c1_pos_m = c1->GetPosition();
      const auto c2_pos_m = c2->GetPosition();

      constexpr auto thickness_m = 0.025f;

      const auto dist = (c2_pos_m - c1_pos_m);
      auto normal = b2Vec2(dist.y, -dist.x);
      normal.Normalize();

      const auto q1 = q1_prev.value_or(c1_pos_m - (thickness_m * normal));
      const auto q2 = c2_pos_m - (thickness_m * normal);
      const auto q3 = c2_pos_m + (thickness_m * normal);
      const auto q4 = q4_prev.value_or(c1_pos_m + (thickness_m * normal));

      q1_prev = q2;
      q4_prev = q3;

      auto u0 = static_cast<float>(i) / static_cast<float>(_segment_count);
      auto u1 = static_cast<float>(i + 1) / static_cast<float>(_segment_count);

      const auto v1 = sf::Vertex(
         sf::Vector2f(q1.x * PPM, q1.y * PPM),
         sf::Color::White,
         sf::Vector2f(
            static_cast<float>(_texture_rect_px.position.x), static_cast<float>(_texture_rect_px.position.y + u0 * _texture_rect_px.size.y)
         )
      );
      const auto v2 = sf::Vertex(
         sf::Vector2f(q2.x * PPM, q2.y * PPM),
         sf::Color::White,
         sf::Vector2f(
            static_cast<float>(_texture_rect_px.position.x), static_cast<float>(_texture_rect_px.position.y + u1 * _texture_rect_px.size.y)
         )
      );
      const auto v3 = sf::Vertex(
         sf::Vector2f(q3.x * PPM, q3.y * PPM),
         sf::Color::White,
         sf::Vector2f(
            static_cast<float>(_texture_rect_px.position.x + _texture_rect_px.size.x),
            static_cast<float>(_texture_rect_px.position.y + u1 * _texture_rect_px.size.y)
         )
      );
      const auto v4 = sf::Vertex(
         sf::Vector2f(q4.x * PPM, q4.y * PPM),
         sf::Color::White,
         sf::Vector2f(
            static_cast<float>(_texture_rect_px.position.x + _texture_rect_px.size.x),
            static_cast<float>(_texture_rect_px.position.y + u0 * _texture_rect_px.size.y)
         )
      );

      // use TriangleStrip ordering
      strip.push_back(v1);
      strip.push_back(v2);
      strip.push_back(v4);
      strip.push_back(v3);
   }

   // render out those quads
   sf::RenderStates states;
   states.texture = _texture.get();
   color.draw(strip.data(), strip.size(), sf::PrimitiveType::TriangleStrip, states);
}

void Rope::pushChain(float impulse)
{
   auto* last_element = _chain_elements.back();
   for (auto* element : _chain_elements)
   {
      element->ApplyLinearImpulse(b2Vec2{impulse, 0.0f}, last_element->GetWorldCenter(), true);
   }
}

void Rope::update(const sf::Time& dt)
{
   // prevent glitches
   // it might make sense to come up with a more high level concept for this
   if (dt.asMilliseconds() > 500)
   {
      return;
   }

   if (!_wind_enabled)
   {
      return;
   }

   if (_player_impulse.has_value() && Player::getCurrent()->getPixelRectFloat().findIntersection(_bounding_box).has_value())
   {
      // using a fix timestep for now, everything else lets box2d go nuts
      const auto impulse = Player::getCurrent()->getBody()->GetLinearVelocity().x * _player_impulse.value() * dt.asSeconds();
      pushChain(impulse);

      // cap speed
      for (auto* body : _chain_bodies)
      {
         const auto& linear_velocity = body->GetLinearVelocity();
         body->SetLinearVelocity({std::clamp(linear_velocity.x, -3.0f, 3.0f), linear_velocity.y});
      }
   }

   // slightly push the rope all the way while it's moving from the right to the left
   _push_time_s += dt.asSeconds();
   if (_push_time_s > _push_interval_s)
   {
      const auto impulse = -_push_strength * dt.asSeconds();
      pushChain(impulse);
   }

   if (_push_time_s > _push_interval_s + _push_duration_s)
   {
      _push_time_s = 0.0f;
   }
}

std::optional<sf::FloatRect> Rope::getBoundingBoxPx()
{
   return _bounding_box;
}

void Rope::setup(const GameDeserializeData& data)
{
   const auto path = data._base_path / "tilesets" / "catacombs-level-diffuse.png";
   _texture = TexturePool::getInstance().get(path);

   // rope 1
   // 971,  73 .. 973,  73
   // 971, 211 .. 973, 211
   _texture_rect_px.position.x = 971;
   _texture_rect_px.position.y = 73;
   _texture_rect_px.size.x = 3;
   _texture_rect_px.size.y = 138;

   // rope 2
   // 1019,  72 .. 1021,  72
   // 1019, 153 .. 1021, 153
   //
   // _texture_rect_px.position.x = 1019;
   // _texture_rect_px.position.y = 72;
   // _texture_rect_px.size.x = 3;
   // _texture_rect_px.size.y = 81;

   // read properties
   const auto push_interval_it = data._tmx_object->_properties->_map.find("push_interval_s");
   if (push_interval_it != data._tmx_object->_properties->_map.end())
   {
      _push_interval_s = push_interval_it->second->_value_float.value();
   }

   const auto push_duration_it = data._tmx_object->_properties->_map.find("push_duration_s");
   if (push_duration_it != data._tmx_object->_properties->_map.end())
   {
      _push_duration_s = push_duration_it->second->_value_float.value();
   }

   const auto push_strength_it = data._tmx_object->_properties->_map.find("push_strength");
   if (push_strength_it != data._tmx_object->_properties->_map.end())
   {
      _push_strength = push_strength_it->second->_value_float.value();
   }

   const auto segment_it = data._tmx_object->_properties->_map.find("segments");
   if (segment_it != data._tmx_object->_properties->_map.end())
   {
      _segment_count = segment_it->second->_value_int.value();
   }

   const auto player_impulse_it = data._tmx_object->_properties->_map.find("player_impulse");
   if (player_impulse_it != data._tmx_object->_properties->_map.end())
   {
      _player_impulse = player_impulse_it->second->_value_float.value();
   }

   // init segment length
   std::vector<sf::Vector2f> pixel_path = data._tmx_object->_polyline->_polyline;
   const auto path_0_px = pixel_path.at(0);
   const auto path_1_px = pixel_path.at(1);
   const auto rope_length_px = path_1_px - path_0_px;
   _segment_length_m = (SfmlMath::length(rope_length_px) * MPP) / static_cast<float>(_segment_count);

   // init start position
   setPixelPosition(sf::Vector2i{
      static_cast<int32_t>(data._tmx_object->_x_px - data._tmx_object->_width_px),
      static_cast<int32_t>(data._tmx_object->_y_px + path_1_px.y)  // the 2nd y coord of the polyline contains the line length
   });

   //      p0
   //   +---+---+
   //   |   |   |
   //   |   |   |
   //   |   |   |
   //   |   |   |
   //   |   |   |
   //   +---+---+
   //      p1
   _bounding_box = sf::FloatRect{{data._tmx_object->_x_px - 10, data._tmx_object->_y_px}, {20, std::fabs(rope_length_px.y)}};

   addChunks(_bounding_box);

   // pin the rope to the starting point (anchor)
   const auto pos_m = b2Vec2{static_cast<float>(_position_px.x * MPP), static_cast<float>(_position_px.y * MPP)};
   _anchor_a_body = data._world->CreateBody(&_anchor_a_def);
   _anchor_a_shape.SetTwoSided(b2Vec2(pos_m.x - 0.1f, pos_m.y), b2Vec2(pos_m.x + 0.1f, pos_m.y));
   auto* anchor_fixture = _anchor_a_body->CreateFixture(&_anchor_a_shape, 0.0f);
   anchor_fixture->SetSensor(true);

   auto* previous_body = _anchor_a_body;

   for (auto i = 0; i < _segment_count; ++i)
   {
      // create chain element
      b2BodyDef chain_body_def;
      chain_body_def.type = b2_dynamicBody;
      chain_body_def.position.Set(pos_m.x, pos_m.y + 0.01f + i * _segment_length_m);
      auto* chain_body = data._world->CreateBody(&chain_body_def);
      auto* chain_fixture = chain_body->CreateFixture(&_rope_element_fixture_def);
      chain_fixture->SetSensor(true);
      _chain_bodies.push_back(chain_body);

      // attach chain element to the previous one
      const b2Vec2 anchor(pos_m.x, pos_m.y + i * _segment_length_m);
      _joint_def.Initialize(previous_body, chain_body, anchor);
      _joint_def.collideConnected = false;
      data._world->CreateJoint(&_joint_def);

      // store chain elements
      previous_body = chain_body;
      _chain_elements.push_back(chain_body);
   }
}

sf::Vector2i Rope::getPixelPosition() const
{
   return _position_px;
}

void Rope::setPixelPosition(const sf::Vector2i& pixel_position)
{
   _position_px = pixel_position;
}
