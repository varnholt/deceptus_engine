#include "spikeball.h"

#include "constants.h"
#include "fixturenode.h"
#include "framework/math/hermitecurve.h"
#include "player/player.h"
#include "framework/tmxparser/tmxobject.h"
#include "texturepool.h"

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


SpikeBall::SpikeBall(GameNode* node)
 : GameNode(node)
{
   setZ(16);

   // chain element setup
   _chain_element_shape.SetAsBox(_config._chain_element_width, _config._chain_element_height);
   _chain_element_fixture_def.shape = &_chain_element_shape;
   _chain_element_fixture_def.density = 20.0f;
   _chain_element_fixture_def.friction = 0.2f;

   _texture = TexturePool::getInstance().get("data/sprites/enemy_spikeball.png");
   _spike_sprite.setTexture(*_texture);
   _spike_sprite.setTextureRect(sf::IntRect(24, 0, 48, 48));
   _spike_sprite.setOrigin(24, 24);

   _box_sprite.setTexture(*_texture);
   _box_sprite.setTextureRect(sf::IntRect(72, 45, 24, 27));
   _box_sprite.setOrigin(12, 15);

   _chain_element_a.setTexture(*_texture);
   _chain_element_a.setTextureRect(sf::IntRect(0, 64, 8, 8));
   _chain_element_a.setOrigin(4, 4);

   _chain_element_b.setTexture(*_texture);
   _chain_element_b.setTextureRect(sf::IntRect(34, 64, 8, 8));
   _chain_element_b.setOrigin(4, 4);
}


void SpikeBall::drawChain(sf::RenderTarget& window)
{
   std::vector<HermiteCurveKey> keys;

   auto t = 0.0f;
   auto ti = 1.0f / _chain_elements.size();
   for (auto c : _chain_elements)
   {
      HermiteCurveKey k;
      k.mPosition = sf::Vector2f{c->GetPosition().x * PPM, c->GetPosition().y * PPM};
      k.mTime = (t += ti);
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
      element.setPosition(point);

      window.draw(element);
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
         auto c1 = _chain_elements[i];
         auto c2 = _chain_elements[i + 1];
         const auto c1_pos_m = c1->GetPosition();
         const auto c2_pos_m = c2->GetPosition();

         sf::Vertex line[] =
         {
            sf::Vertex(sf::Vector2f(c1_pos_m.x * PPM, c1_pos_m.y * PPM), vertex_color),
            sf::Vertex(sf::Vector2f(c2_pos_m.x * PPM, c2_pos_m.y * PPM), vertex_color),
         };

         color.draw(line, 2, sf::Lines);
         // printf("draw %d: %f, %f -> %f, %f\n", i, c1Pos.x * PPM, c1Pos.y * PPM, c2Pos.x * PPM, c2Pos.y * PPM);
      }
   }

   color.draw(_box_sprite);
   drawChain(color);
   color.draw(_spike_sprite);
}


void SpikeBall::update(const sf::Time& dt)
{
   _spike_sprite.setPosition(
      _ball_body->GetPosition().x * PPM,
      _ball_body->GetPosition().y * PPM
   );

   static const b2Vec2 up{0.0, 1.0};

   auto c1_pos_m = _chain_elements[0]->GetPosition();
   auto c2_pos_m = _chain_elements[static_cast<size_t>(_config._chain_element_count - 1)]->GetPosition();

   auto c_dist_m = (c2_pos_m - c1_pos_m);
   c_dist_m.Normalize();

   // _angle = acos(b2Dot(up, c_dist_m) / (c_dist_m.Length() * up.Length()));
   _angle = acos(b2Dot(up, c_dist_m) / (c_dist_m.LengthSquared() * up.LengthSquared()));

   if (c_dist_m.x > 0.0f)
   {
      _angle = -_angle;
   }

   _spike_sprite.setRotation(_angle * RADTODEG);

   // slightly push the ball all the way while it's moving from the right to the left
   auto f = dt.asSeconds() * _config._push_factor;
   if (_ball_body->GetLinearVelocity().x < 0.0f)
   {
      _ball_body->ApplyLinearImpulse(b2Vec2{-f, f}, _ball_body->GetWorldCenter(), true);
   }
}


void SpikeBall::setup(TmxObject* tmx_object, const std::shared_ptr<b2World>& world)
{
    setPixelPosition(
       sf::Vector2i{
          static_cast<int32_t>(tmx_object->mX),
          static_cast<int32_t>(tmx_object->mY)
       }
    );

   auto pos = b2Vec2{static_cast<float>(_pixel_position.x * MPP), static_cast<float>(_pixel_position.y * MPP)};

   _anchor_body = world->CreateBody(&_anchor_def);
   _anchor_shape.Set(b2Vec2(pos.x - 0.1f, pos.y), b2Vec2(pos.x + 0.1f, pos.y));
   _anchor_body->CreateFixture(&_anchor_shape, 0.0f);

   _joint_def.collideConnected = false;

   auto prev_body = _anchor_body;
   for (auto i = 0; i < _config._chain_element_count; ++i)
   {
      b2BodyDef bd;
      bd.type = b2_dynamicBody;
      bd.position.Set(pos.x + 0.01f + i * _config._chain_element_distance, pos.y);
      auto chain_body = world->CreateBody(&bd);
      auto chain_fixture = chain_body->CreateFixture(&_chain_element_fixture_def);
      chain_fixture->SetSensor(true);
      _chain_elements.push_back(chain_body);

      b2Vec2 anchor(pos.x + i * _config._chain_element_distance, pos.y);

      _joint_def.Initialize(prev_body, chain_body, anchor);
      world->CreateJoint(&_joint_def);

      prev_body = chain_body;
   }

   // attach the spiky ball to the last chain element
   _ball_body_def.type = b2_dynamicBody;
   _ball_fixture_def.density = 1;
   _ball_shape.m_radius = 0.45f;
   _ball_body_def.position.Set(pos.x + 0.01f + _config._chain_element_count * _config._chain_element_distance, pos.y);
   _ball_fixture_def.shape = &_ball_shape;
   _ball_body = world->CreateBody( &_ball_body_def );
   auto ball_fixture = _ball_body->CreateFixture( &_ball_fixture_def );
   b2Vec2 anchor(pos.x + _config._chain_element_count * _config._chain_element_distance, pos.y);
   _joint_def.Initialize(prev_body, _ball_body, anchor);
   world->CreateJoint(&_joint_def);

   auto object_data = new FixtureNode(this);
   object_data->setType(ObjectTypeDeadly);
   ball_fixture->SetUserData(static_cast<void*>(object_data));

   // that box only needs to be set up once
   _box_sprite.setPosition(
      _chain_elements[0]->GetPosition().x * PPM,
      _chain_elements[0]->GetPosition().y * PPM
   );
}


sf::Vector2i SpikeBall::getPixelPosition() const
{
   return _pixel_position;
}


void SpikeBall::setPixelPosition(const sf::Vector2i& pixelPosition)
{
   _pixel_position = pixelPosition;
}


