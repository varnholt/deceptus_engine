#include "rope.h"

#include "framework/math/sfmlmath.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolyline.h"


Rope::Rope(GameNode* parent)
 : GameNode(parent)
{
   _joint_def.collideConnected = false;

   setZ(16);

   // chain element setup
   _rope_element_shape.SetAsBox(0.0125f, 0.0125f);
   _rope_element_fixture_def.shape = &_rope_element_shape;
   _rope_element_fixture_def.density = 20.0f;
   _rope_element_fixture_def.friction = 0.2f;
}


void Rope::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   static const auto vertex_color = sf::Color(200, 200, 240);
   static const bool draw_debug_line = true;

   if (draw_debug_line)
   {
      for (auto i = 0u; i < _chain_elements.size() - 1; i++)
      {
         const auto c1 = _chain_elements[i];
         const auto c2 = _chain_elements[i + 1];

         const auto c1_pos_m = c1->GetPosition();
         const auto c2_pos_m = c2->GetPosition();

         sf::Vertex line[] =
         {
            sf::Vertex(sf::Vector2f(c1_pos_m.x * PPM, c1_pos_m.y * PPM), vertex_color),
            sf::Vertex(sf::Vector2f(c2_pos_m.x * PPM, c2_pos_m.y * PPM), vertex_color),
         };

         color.draw(line, 2, sf::Lines);
      }
   }

//   color.draw(_box_sprite);
//   drawChain(color);
//   color.draw(_spike_sprite);
}


void Rope::update(const sf::Time& dt)
{
   // slightly push the rope all the way while it's moving from the right to the left
   auto f = dt.asSeconds() * 0.001f;
   auto last_element = _chain_elements.at(_chain_elements.size() - 1);
   if (last_element->GetLinearVelocity().x < 0.0f)
   {
      last_element->ApplyLinearImpulse(b2Vec2{-f, f}, last_element->GetWorldCenter(), true);
   }
}


void Rope::setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& world)
{
   // init segment length
   std::vector<sf::Vector2f> pixel_path = tmxObject->mPolyLine->mPolyLine;
   auto path_0_px = pixel_path.at(0);
   auto path_1_px = pixel_path.at(1);
   _segment_length_m = (SfmlMath::length(path_1_px - path_0_px) * MPP) / static_cast<float>(_segment_count);

   // init start position
   setPixelPosition(
      sf::Vector2i{
         static_cast<int32_t>(tmxObject->mX),
         static_cast<int32_t>(tmxObject->mY)
      }
   );

   // pin the rope to the starting point (anchor)
   auto pos_m = b2Vec2{static_cast<float>(_position_px.x * MPP), static_cast<float>(_position_px.y * MPP)};
   _anchor_a_body = world->CreateBody(&_anchor_a_def);
   _anchor_a_shape.Set(b2Vec2(pos_m.x - 0.1f, pos_m.y), b2Vec2(pos_m.x + 0.1f, pos_m.y));
   _anchor_a_body->CreateFixture(&_anchor_a_shape, 0.0f);

   auto previous_body = _anchor_a_body;

   for (auto i = 0; i < _segment_count; ++i)
   {
      // create chain element
      b2BodyDef chain_body_def;
      chain_body_def.type = b2_dynamicBody;
      chain_body_def.position.Set(pos_m.x + 0.01f + i * _segment_length_m, pos_m.y);
      auto chain_body = world->CreateBody(&chain_body_def);
      auto chain_fixture = chain_body->CreateFixture(&_rope_element_fixture_def);
      chain_fixture->SetSensor(true);

      // attach chain element to the previous one
      b2Vec2 anchor(pos_m.x + i * _segment_length_m, pos_m.y);
      _joint_def.Initialize(previous_body, chain_body, anchor);
      world->CreateJoint(&_joint_def);

      // store chain elements
      previous_body = chain_body;
      _chain_elements.push_back(chain_body);
   }
}


sf::Vector2i Rope::getPixelPosition() const
{
   return _position_px;
}


void Rope::setPixelPosition(const sf::Vector2i& pixelPosition)
{
   _position_px = pixelPosition;
}


