#include "watersurface.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"
#include "player/player.h"

#include <iostream>

namespace
{

float tension = 0.025f;
float dampening = 0.025f;
float spread = 0.25f;

constexpr auto animation_speed = 10.0f;
constexpr auto splash_factor = 50.0f;

}  // namespace

void WaterSurface::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   //
   //         __--4
   //   __- 2-    |
   // 0-    |\    |
   // | \   | \   |
   // |  \  |  \  |
   // |   \ |   \ |
   // +-----+-----+
   // 1     3     5
   //

   // draw water color
   sf::RenderStates states;
   states.texture = &_gradient;

   if (_pixel_ratio.has_value())
   {
      states.blendMode = sf::BlendAlpha;
      _render_texture.clear({0, 0, 0, 0});
      _render_texture.draw(_vertices, states);
      _render_texture.display();
      color.draw(render_texture_sprite, states);
   }
   else
   {
      color.draw(_vertices, states);
   }

   // draw lines
   constexpr auto draw_lines = false;
   if (draw_lines)
   {
      auto index = 0;
      const auto segment_width = _bounding_box.width / _segments.size();
      std::vector<sf::Vertex> sf_lines;
      const auto x_offset = _bounding_box.left;
      const auto y_offset = _bounding_box.top;

      for (const auto& segment : _segments)
      {
         const auto x = x_offset + static_cast<float>(index * segment_width);
         const auto y = y_offset + segment._height;
         sf_lines.push_back(sf::Vertex{sf::Vector2f{x, y}, sf::Color::White});
         index++;
      }

      color.draw(sf_lines.data(), sf_lines.size(), sf::LineStrip);
   }
}


void WaterSurface::update(const sf::Time& dt)
{
   auto player = Player::getCurrent();

   //   // just to save some performance
   //   if (!player->isInWater())
   //   {
   //      return;
   //   }

   bool splash_needed = false;

   // initialize 'player is in water' so it triggers only when the player crosses the surface
   if (!_player_was_in_water.has_value())
   {
      _player_was_in_water = player->isInWater();
   }

   if (_player_was_in_water != player->isInWater())
   {
      _player_was_in_water = player->isInWater();
      splash_needed = true;
   }

   if (splash_needed)
   {
      sf::FloatRect intersection;
      if (player->getPixelRectFloat().intersects(_bounding_box, intersection))
      {
         const auto velocity = player->getBody()->GetLinearVelocity().y * splash_factor;
         const auto normalized_intersection = (intersection.left - _bounding_box.left) / _bounding_box.width;
         const auto index = static_cast<int32_t>(normalized_intersection * _segments.size());

         splash(index, velocity);
      }
   }

   for (auto& segment : _segments)
   {
      segment.update(dampening, tension);
      segment.resetDeltas();
   }

   static constexpr auto integration_steps = 8;

   // integrate a few times
   for (auto j = 0; j < integration_steps; j++)
   {
      for (auto segment_index = 0; segment_index < _segments.size(); segment_index++)
      {
         if (segment_index > 0)
         {
            const auto delta_left =
               spread * (_segments[segment_index]._height - _segments[segment_index - 1]._height) * dt.asSeconds() * animation_speed;

            _segments[segment_index]._delta_left = delta_left;
            _segments[segment_index - 1]._velocity += delta_left;
         }

         if (segment_index < _segments.size() - 1)
         {
            const auto delta_right =
               spread * (_segments[segment_index]._height - _segments[segment_index + 1]._height) * dt.asSeconds() * animation_speed;

            _segments[segment_index]._delta_right = delta_right;
            _segments[segment_index + 1]._velocity += delta_right;
         }
      }

      // update heights based on deltas
      for (auto segment_index = 0; segment_index < _segments.size(); segment_index++)
      {
         if (segment_index > 0)
         {
            _segments[segment_index - 1]._height += _segments[segment_index]._delta_left;
         }

         if (segment_index < _segments.size() - 1)
         {
            _segments[segment_index + 1]._height += _segments[segment_index]._delta_right;
         }
      }
   }

   // only update the top parts of the poly because the bottom doesn't move
   updateVertices(0);
}

std::optional<sf::FloatRect> WaterSurface::getBoundingBoxPx()
{
   return _bounding_box;
}

void WaterSurface::Segment::update(float dampening, float tension)
{
   const auto x = _target_height - _height;
   _velocity += tension * x - _velocity * dampening;
   _height += _velocity;
}

void WaterSurface::Segment::resetDeltas()
{
   _delta_left = 0.0f;
   _delta_right = 0.0f;
}

void WaterSurface::splash(int32_t index, float velocity)
{
   const auto start_index = std::max(0, index);
   const auto stop_index = std::min<size_t>(_segments.size() - 1, index + 1);

   for (auto i = start_index; i < stop_index; i++)
   {
      _segments[index]._velocity = velocity;
   }
}

void WaterSurface::updateVertices(int32_t start_index)
{
   constexpr auto increment = 2;
   auto index = start_index;
   auto width_index = 0;

   auto x_offset = 0.0f;
   auto y_offset = 0.0f;
   if (!_pixel_ratio.has_value())
   {
      x_offset = _bounding_box.left;
      y_offset = _bounding_box.top;
   }

   for (const auto& segment : _segments)
   {
      const auto x = x_offset + static_cast<float>(width_index * _segment_width);
      const auto y = (index & 1) ? (y_offset + _bounding_box.height) : (y_offset + segment._height);

      _vertices[index].position.x = x;
      _vertices[index].position.y = y;

      // should be done just upon init
      _vertices[index].texCoords.x = (index & 1) ? 255.0f : 0.0f;
      _vertices[index].color.a = 200;

      index += increment;
      width_index++;
   }
}

WaterSurface::WaterSurface(GameNode* parent, const GameDeserializeData& data)
{
   setClassName(typeid(WaterSurface).name());
   setObjectId(data._tmx_object->_name);

   _bounding_box.left = data._tmx_object->_x_px;
   _bounding_box.top = data._tmx_object->_y_px;
   _bounding_box.width = data._tmx_object->_width_px;
   _bounding_box.height = data._tmx_object->_height_px;

   auto segment_count = static_cast<int32_t>(_bounding_box.width / 2);

   // read properties
   if (data._tmx_object->_properties)
   {
      auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         setZ(static_cast<uint32_t>(z_it->second->_value_int.value()));
      }

      auto segment_it = data._tmx_object->_properties->_map.find("segment_count");
      if (segment_it != data._tmx_object->_properties->_map.end())
      {
         segment_count = static_cast<int32_t>(segment_it->second->_value_int.value());
      }

      auto pixel_ratio_it = data._tmx_object->_properties->_map.find("pixel_ratio");
      if (pixel_ratio_it != data._tmx_object->_properties->_map.end())
      {
         _pixel_ratio = static_cast<int32_t>(pixel_ratio_it->second->_value_int.value());
      }
   }

   for (auto i = 0; i < segment_count; i++)
   {
      _segments.push_back({});
   }

   _vertices.setPrimitiveType(sf::PrimitiveType::TriangleStrip);
   _vertices.resize(segment_count * 2);

   _segment_width = _bounding_box.width / _segments.size();

   updateVertices(0);
   updateVertices(1);

   const auto box_width = static_cast<int32_t>(_bounding_box.width);
   if (box_width % segment_count != 0)
   {
      Log::Error() << "box with width " << box_width << "px cannot be divided into " << segment_count << " segments" << std::endl;
   }

   _gradient.loadFromFile("data/sprites/water_surface_gradient.png");

   Log::Info() << "deserialize water surface at: " << _bounding_box.left << ", " << _bounding_box.top << " w: " << _bounding_box.width
               << ", h:" << _bounding_box.height << std::endl;

   // if a pixel ratio is configured, we gotta render to texture
   if (_pixel_ratio.has_value())
   {
      if (!_render_texture.create(
             static_cast<int32_t>(_bounding_box.width / _pixel_ratio.value()),
             static_cast<int32_t>((_bounding_box.height * 2) / _pixel_ratio.value())
          ))
      {
         Log::Error() << "could not create render texture";
      }

      _render_texture.setSmooth(false);
      render_texture_sprite.setTexture(_render_texture.getTexture());
      render_texture_sprite.setPosition({_bounding_box.left, _bounding_box.top});
      render_texture_sprite.scale(_pixel_ratio.value(), _pixel_ratio.value());
   }
}
