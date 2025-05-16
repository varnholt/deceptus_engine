#include "spikeball.h"

#include <iostream>

#include "framework/math/hermitecurve.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/audio/audio.h"
#include "game/constants.h"
#include "game/io/texturepool.h"
#include "game/level/fixturenode.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/player.h"

namespace
{
const auto registered_spikeball = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.mapGroupToLayer("SpikeBall", "spike_balls");

   registry.registerLayerName(
      "spike_balls",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<SpikeBall>(parent);
         mechanism->setup(data);
         mechanisms["spike_balls"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "SpikeBall",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<SpikeBall>(parent);
         mechanism->setup(data);
         mechanisms["spike_balls"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

namespace
{
auto instance_counter = 0;
constexpr auto box_sprite_y_offset_px = -3;
}  // namespace

/*
   spike ball concept

                        +-----------+
                        |           |
                        |     x     |    box body + rotating revolute joint (static body)
                        |   ./      |
                        +-./--------+
                        ./               thin box body + distance joint      _________   _________   _________
                      ./                 thin box body + distance joint    -[-o     o-]-[-o     o-]-[-o     o-]-
                    ./                   thin box body + distance joint     '---------' '---------' '---------'
              \- __^_ -/
               .`    '.
             < : O  x : >                circular body (bad spiky ball, dynamic body)
               :  __  :
              /_`----'_\
                  \/

   https://www.iforce2d.net/b2dtut/joints-revolute
*/

SpikeBall::SpikeBall(GameNode* parent) : GameNode(parent), _instance_id(instance_counter++)
{
   _reference_volume = 1.0f;
   _audio_update_data._range = AudioRange{800.0f, 0.0f, 100.0f, _reference_volume};
   _has_audio = true;

   setClassName(typeid(SpikeBall).name());
   setZ(16);

   // chain element setup
   _chain_element_shape.SetAsBox(_config._chain_element_width, _config._chain_element_height);
   _chain_element_fixture_def.shape = &_chain_element_shape;
   _chain_element_fixture_def.density = 20.0f;
   _chain_element_fixture_def.friction = 0.2f;

   _texture = TexturePool::getInstance().get("data/sprites/enemy_spikeball.png");

   _spike_sprite = std::make_unique<sf::Sprite>(*_texture);
   _box_sprite = std::make_unique<sf::Sprite>(*_texture);
   _chain_element_a = std::make_unique<sf::Sprite>(*_texture);
   _chain_element_b = std::make_unique<sf::Sprite>(*_texture);

   _spike_sprite->setTextureRect(sf::IntRect({118, 24}, {51, 50}));
   _spike_sprite->setOrigin({25, 25});

   _box_sprite->setTextureRect(sf::IntRect({168, 93}, {24, 27}));

   _chain_element_a->setTextureRect(sf::IntRect({297, 56}, {8, 8}));
   _chain_element_a->setOrigin({4, 4});

   _chain_element_b->setTextureRect(sf::IntRect({320, 56}, {8, 8}));
   _chain_element_b->setOrigin({4, 4});
}

void SpikeBall::preload()
{
   Audio::getInstance().addSample("mechanism_spikeball_01.wav");
   Audio::getInstance().addSample("mechanism_spikeball_02.wav");
}

void SpikeBall::drawChain(sf::RenderTarget& window)
{
   std::vector<HermiteCurveKey> keys;

   auto t = 0.0f;
   auto ti = 1.0f / _chain_elements.size();
   for (auto* c : _chain_elements)
   {
      HermiteCurveKey k;
      k._position = sf::Vector2f{c->GetPosition().x * PPM, c->GetPosition().y * PPM};
      k._time = (t += ti);
      keys.push_back(k);
   }

   HermiteCurve curve;
   curve.setPositionKeys(keys);
   curve.compute();

   auto val = 0.0f;
   auto increment = 1.0f / _config._spline_point_count;
   for (auto i = 0; i < _config._spline_point_count; i++)
   {
      auto point = curve.computePoint(val += increment);

      auto& element = (i % 2 == 0) ? _chain_element_a : _chain_element_b;
      element->setPosition(point);

      window.draw(*element);
   }
}

void SpikeBall::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   static const auto vertex_color = sf::Color(200, 200, 240);
   static const bool draw_debug_line = false;

   if (draw_debug_line)
   {
      for (auto i = 0u; i < _chain_elements.size() - 1; i++)
      {
         auto* chain_element_1 = _chain_elements[i];
         auto* chain_element_2 = _chain_elements[i + 1];
         const auto c1_pos_m = chain_element_1->GetPosition();
         const auto c2_pos_m = chain_element_2->GetPosition();

         sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(c1_pos_m.x * PPM, c1_pos_m.y * PPM), vertex_color),
            sf::Vertex(sf::Vector2f(c2_pos_m.x * PPM, c2_pos_m.y * PPM), vertex_color),
         };

         color.draw(line, 2, sf::PrimitiveType::Lines);

         // printf("draw %d: %f, %f -> %f, %f\n", i, c1Pos.x * PPM, c1Pos.y * PPM, c2Pos.x * PPM, c2Pos.y * PPM);
      }
   }

   // dstar doesn't want the box sprite to be drawn
   // color.draw(_box_sprite);

   drawChain(color);
   color.draw(*_spike_sprite);
}

void SpikeBall::update(const sf::Time& dt)
{
   if (dt.asMilliseconds() > 16 * 2)
   {
      return;
   }

   _spike_sprite->setPosition({_ball_body->GetPosition().x * PPM, _ball_body->GetPosition().y * PPM});

   static const b2Vec2 up{0.0, 1.0};

   const auto c1_pos_m = _chain_elements[0]->GetPosition();
   const auto c2_pos_m = _chain_elements[static_cast<size_t>(_config._chain_element_count - 1)]->GetPosition();

   auto c_dist_m = (c2_pos_m - c1_pos_m);
   c_dist_m.Normalize();

   _angle = acos(b2Dot(up, c_dist_m) / (c_dist_m.LengthSquared() * up.LengthSquared()));

   if (c_dist_m.x > 0.0f)
   {
      _angle = -_angle;
   }

   const auto angle = sf::radians(_angle);
   _spike_sprite->setRotation(angle);

   // play swoosh sound on every direction change
   if (_audio_enabled)
   {
      const auto changed_direction = std::signbit(_last_ball_x_velocity) != std::signbit(_ball_body->GetLinearVelocity().x);
      if (changed_direction)
      {
         const auto sample = (_swing_counter++ & 1) ? Audio::PlayInfo{"mechanism_spikeball_01.wav", _audio_update_data._volume}
                                                    : Audio::PlayInfo{"mechanism_spikeball_02.wav", _audio_update_data._volume};
         Audio::getInstance().playSample(sample);
      }

      _last_ball_x_velocity = _ball_body->GetLinearVelocity().x;
   }

   // slightly push the ball all the way while it's moving from the right to the left
   if (_ball_body->GetLinearVelocity().x < 0.0f)
   {
      const auto f = dt.asSeconds() * _config._push_factor;
      _ball_body->ApplyLinearImpulse(b2Vec2{-f, f}, _ball_body->GetWorldCenter(), true);
   }
}

std::optional<sf::FloatRect> SpikeBall::getBoundingBoxPx()
{
   return _rect;
}

void SpikeBall::setup(const GameDeserializeData& data)
{
   _rect = sf::FloatRect{{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   addChunks(_rect);

   if (data._tmx_object->_properties)
   {
      auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         setZ(static_cast<uint32_t>(z_it->second->_value_int.value()));
      }

      auto push_factor_it = data._tmx_object->_properties->_map.find("push_factor");
      if (push_factor_it != data._tmx_object->_properties->_map.end())
      {
         _config._push_factor = push_factor_it->second->_value_float.value();
      }

      auto spline_point_count_it = data._tmx_object->_properties->_map.find("spline_point_count");
      if (spline_point_count_it != data._tmx_object->_properties->_map.end())
      {
         _config._spline_point_count = spline_point_count_it->second->_value_int.value();
      }

      auto chain_element_count_it = data._tmx_object->_properties->_map.find("chain_element_count");
      if (chain_element_count_it != data._tmx_object->_properties->_map.end())
      {
         _config._chain_element_count = chain_element_count_it->second->_value_int.value();
      }

      auto chain_element_distance_it = data._tmx_object->_properties->_map.find("chain_element_distance_m");
      if (chain_element_distance_it != data._tmx_object->_properties->_map.end())
      {
         _config._chain_element_distance = chain_element_distance_it->second->_value_float.value();
      }

      auto chain_element_width_it = data._tmx_object->_properties->_map.find("chain_element_width_m");
      if (chain_element_width_it != data._tmx_object->_properties->_map.end())
      {
         _config._chain_element_width = chain_element_width_it->second->_value_float.value();
      }

      auto chain_element_height_it = data._tmx_object->_properties->_map.find("chain_element_height_m");
      if (chain_element_height_it != data._tmx_object->_properties->_map.end())
      {
         _config._chain_element_height = chain_element_height_it->second->_value_float.value();
      }

      auto audio_update_behavior_it = data._tmx_object->_properties->_map.find("audio_update_behavior");
      if (audio_update_behavior_it != data._tmx_object->_properties->_map.end())
      {
         const auto audio_update_behavior_str = audio_update_behavior_it->second->_value_string.value();
         if (audio_update_behavior_str == "room_based")
         {
            _audio_update_data._update_behavior = AudioUpdateBehavior::RoomBased;
         }
         else if (audio_update_behavior_str == "range_based")
         {
            _audio_update_data._update_behavior = AudioUpdateBehavior::RangeBased;
         }
      }
   }

   setPixelPosition(sf::Vector2i{
      static_cast<int32_t>(data._tmx_object->_x_px) + PIXELS_PER_HALF_TILE,
      static_cast<int32_t>(data._tmx_object->_y_px) + PIXELS_PER_HALF_TILE
   });

   const auto pos_m = b2Vec2{static_cast<float>(_pixel_position.x * MPP), static_cast<float>(_pixel_position.y * MPP)};

   _anchor_body = data._world->CreateBody(&_anchor_def);
   _anchor_shape.SetTwoSided(b2Vec2(pos_m.x - 0.1f, pos_m.y), b2Vec2(pos_m.x + 0.1f, pos_m.y));

   b2FixtureDef fd;
   fd.shape = &_anchor_shape;
   fd.filter.groupIndex = 0;
   fd.filter.maskBits = CategoryBoundary;
   fd.filter.categoryBits = CategoryEnemyWalkThrough;
   _anchor_body->CreateFixture(&fd);

   _joint_def.collideConnected = false;

   auto* prev_body = _anchor_body;
   for (auto i = 0; i < _config._chain_element_count; ++i)
   {
      b2BodyDef bd;
      bd.type = b2_dynamicBody;
      bd.position.Set(pos_m.x + 0.01f + i * _config._chain_element_distance, pos_m.y);
      auto* chain_body = data._world->CreateBody(&bd);
      auto* chain_fixture = chain_body->CreateFixture(&_chain_element_fixture_def);
      chain_fixture->SetSensor(true);
      _chain_elements.push_back(chain_body);

      const b2Vec2 anchor(pos_m.x + i * _config._chain_element_distance, pos_m.y);

      _joint_def.Initialize(prev_body, chain_body, anchor);
      data._world->CreateJoint(&_joint_def);

      prev_body = chain_body;
   }

   // attach the spiky ball to the last chain element
   _ball_body_def.type = b2_dynamicBody;
   _ball_fixture_def.density = 1;
   _ball_shape.m_radius = _config._ball_radius;
   _ball_body_def.position.Set(pos_m.x + 0.01f + _config._chain_element_count * _config._chain_element_distance, pos_m.y);
   _ball_fixture_def.shape = &_ball_shape;
   _ball_body = data._world->CreateBody(&_ball_body_def);
   auto* ball_fixture = _ball_body->CreateFixture(&_ball_fixture_def);
   b2Vec2 anchor(pos_m.x + _config._chain_element_count * _config._chain_element_distance, pos_m.y);
   _joint_def.Initialize(prev_body, _ball_body, anchor);
   data._world->CreateJoint(&_joint_def);

   auto* object_data = new FixtureNode(this);
   object_data->setType(ObjectTypeDeadly);
   ball_fixture->SetUserData(static_cast<void*>(object_data));

   // that box only needs to be set up once
   _box_sprite->setPosition({data._tmx_object->_x_px, data._tmx_object->_y_px + box_sprite_y_offset_px});
}

sf::Vector2i SpikeBall::getPixelPosition() const
{
   return _pixel_position;
}

void SpikeBall::setPixelPosition(const sf::Vector2i& pixel_position)
{
   _pixel_position = pixel_position;
}
