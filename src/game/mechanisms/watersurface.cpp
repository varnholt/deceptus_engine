#include "watersurface.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"
#include "game/debugdraw.h"
#include "player/player.h"

#include <iostream>

namespace
{

float frand(float min = 0.0f, float max = 1.0f)
{
   const auto val = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
   return min + val * (max - min);
}

std::vector<WaterSurface*> surfaces;
std::vector<WaterSurface::SplashEmitter> emitters;

}  // namespace

// #define DEBUG_WATERSURFACE 1

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

   // draw water gradient
   sf::RenderStates states;
   states.texture = &_gradient;

   if (_pixel_ratio.has_value())
   {
      states.blendMode = sf::BlendAlpha;
      _render_texture.clear({0, 0, 0, 0});
#ifdef DEBUG_WATERSURFACE
      _render_texture.clear({255, 0, 0, 200});
#endif
      _render_texture.draw(_vertices, states);
      _render_texture.display();
      color.draw(render_texture_sprite, states);
   }
   else
   {
      color.draw(_vertices, states);
   }

#ifdef DEBUG_WATERSURFACE
   DebugDraw::drawRect(color, _bounding_box);
#endif

#ifdef DEBUG_WATERSURFACE
   auto index = 0;
   const auto segment_width = _bounding_box.width / (_segments.size() - 1);
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
#endif
}

void WaterSurface::update(const sf::Time& dt)
{
   // safeguard against me debugging this :)
   if (dt.asMilliseconds() > 16 * 2)
   {
      return;
   }

   const auto elapsed_s = dt.asSeconds();
   updateEmitters(elapsed_s);

   auto player = Player::getCurrent();

   bool splash_needed = false;
   auto splash_velocity_factor = 1.0f;

   // initialize 'player is in water' so it triggers only when the player crosses the surface
   if (!_player_was_in_water.has_value())
   {
      _player_was_in_water = player->isInWater();
   }

   if (_player_was_in_water != player->isInWater())
   {
      if (_player_was_in_water.value())
      {
         // the wave amplitude should be smaller when leaving the water
         splash_velocity_factor = 0.6f;
      }

      _player_was_in_water = player->isInWater();
      splash_needed = true;
   }

   if (splash_needed)
   {
      sf::FloatRect intersection;
      if (player->getPixelRectFloat().intersects(_bounding_box, intersection))
      {
         const auto velocity = splash_velocity_factor * player->getBody()->GetLinearVelocity().y * _config._splash_factor;
         const auto normalized_intersection =
            (intersection.left + (player->getPixelRectFloat().width / 2.0f) - _bounding_box.left) / _bounding_box.width;
         const auto index = static_cast<int32_t>(normalized_intersection * _segments.size());

         splash(index, velocity);
      }
   }

   for (auto& segment : _segments)
   {
      segment.update(_config._dampening, _config._tension);
      segment.resetDeltas();
   }

   // integrate a few times
   static constexpr auto integration_steps = 8;
   for (auto j = 0; j < integration_steps; j++)
   {
      for (auto segment_index = 0; segment_index < _segments.size(); segment_index++)
      {
         if (segment_index > 0)
         {
            const auto delta_left = _config._spread * (_segments[segment_index]._height - _segments[segment_index - 1]._height) *
                                    elapsed_s * _config._animation_speed;

            _segments[segment_index]._delta_left = delta_left;
            _segments[segment_index - 1]._velocity += delta_left;
         }

         if (segment_index < _segments.size() - 1)
         {
            const auto delta_right = _config._spread * (_segments[segment_index]._height - _segments[segment_index + 1]._height) *
                                     elapsed_s * _config._animation_speed;

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

void WaterSurface::addEmitter(GameNode* parent, const GameDeserializeData& data)
{
   SplashEmitter emitter;

   emitter._bounding_box.left = data._tmx_object->_x_px;
   emitter._bounding_box.top = data._tmx_object->_y_px;
   emitter._bounding_box.width = data._tmx_object->_width_px;
   emitter._bounding_box.height = data._tmx_object->_height_px;

   if (data._tmx_object->_properties)
   {
      // figure out the emitter's parent
      auto ref_it = data._tmx_object->_properties->_map.find("reference");
      if (ref_it != data._tmx_object->_properties->_map.end())
      {
         emitter._parent_reference = ref_it->second->_value_string.value();
      }

      // read emitter properties
      auto readFloatValue = [&](const std::string& key) -> std::optional<float>
      {
         auto it = data._tmx_object->_properties->_map.find(key);
         if (it != data._tmx_object->_properties->_map.end())
         {
            return it->second->_value_float.value();
         }
         return std::nullopt;
      };

      emitter._x_from_px = readFloatValue("x_from_px").value_or(0.0f);
      emitter._x_to_px = readFloatValue("x_to_px").value_or(data._tmx_object->_width_px);
      emitter._interval_min_s = readFloatValue("interval_min_s").value_or(0.3f);
      emitter._interval_max_s = readFloatValue("interval_max_s").value_or(0.6f);
      emitter._velocity = readFloatValue("velocity").value_or(2.0f);
      emitter._width_px = readFloatValue("width_px").value_or(data._tmx_object->_width_px);

      auto count_it = data._tmx_object->_properties->_map.find("count");
      if (count_it != data._tmx_object->_properties->_map.end())
      {
         emitter._count = count_it->second->_value_int.value();
      }
   }

   emitters.push_back(emitter);
}

void WaterSurface::merge()
{
   for (auto& emitter : emitters)
   {
      const auto it = std::find_if(
         surfaces.begin(), surfaces.end(), [emitter](const auto& surface) { return emitter._parent_reference == surface->getObjectId(); }
      );

      if (it != surfaces.end())
      {
         auto surface = *it;
         emitter._x_offset_to_parent_px = emitter._bounding_box.left - surface->_bounding_box.left;
         surface->_emitters.push_back(emitter);
      }
      else
      {
         Log::Error() << "water surface emitter with reference '" << emitter._parent_reference << "' is orphaned";
      }
   }

   emitters.clear();
   surfaces.clear();
}

void WaterSurface::updateVertices(int32_t start_index)
{
   constexpr auto increment = 2;  // we either update all upper or all lower vertices
   auto index = start_index;
   auto width_index = 0;

   auto x_offset = 0.0f;
   auto y_offset = 0.0f;
   if (!_pixel_ratio.has_value())
   {
      x_offset = _bounding_box.left;
      y_offset = _bounding_box.top;
   }
   else
   {
      y_offset = _bounding_box.height / _pixel_ratio.value();
   }

   for (const auto& segment : _segments)
   {
      const auto x = x_offset + static_cast<float>(width_index * _segment_width);
      const auto y = (index & 1) ? (y_offset + _bounding_box.height) : (y_offset + segment._height * segment._clamp_scale);

      _vertices[index].position.x = x;
      _vertices[index].position.y = y;

      // should be done just upon init
      _vertices[index].texCoords.x = (index & 1) ? 48.0f : 0.0f;
      _vertices[index].color.a = _opacity;

      index += increment;
      width_index++;
   }
}

WaterSurface::WaterSurface(GameNode* parent, const GameDeserializeData& data)
{
   setClassName(typeid(WaterSurface).name());
   setObjectId(data._tmx_object->_name);

   surfaces.push_back(this);

   _bounding_box.left = data._tmx_object->_x_px;
   _bounding_box.top = data._tmx_object->_y_px;
   _bounding_box.width = data._tmx_object->_width_px;
   _bounding_box.height = data._tmx_object->_height_px;

   _chunks.push_back({_bounding_box.left, _bounding_box.top});
   _chunks.push_back({_bounding_box.left, _bounding_box.top + _bounding_box.height});
   _chunks.push_back({_bounding_box.left + _bounding_box.width, _bounding_box.top + _bounding_box.height});
   _chunks.push_back({_bounding_box.left + _bounding_box.width, _bounding_box.top});

   auto segment_count = static_cast<int32_t>(_bounding_box.width / 2);
   std::optional<int32_t> clamp_segment_count;

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

         if (segment_count > _bounding_box.width)
         {
            Log::Error() << "segment_count " << segment_count << " exceeds bounding box width " << _bounding_box.width << std::endl;
         }
      }

      auto pixel_ratio_it = data._tmx_object->_properties->_map.find("pixel_ratio");
      if (pixel_ratio_it != data._tmx_object->_properties->_map.end())
      {
         _pixel_ratio = pixel_ratio_it->second->_value_float.value();
         segment_count = static_cast<int32_t>(static_cast<float>(segment_count) / _pixel_ratio.value());
      }

      auto clamp_segment_count_it = data._tmx_object->_properties->_map.find("clamp_segment_count");
      if (clamp_segment_count_it != data._tmx_object->_properties->_map.end())
      {
         clamp_segment_count = static_cast<int32_t>(clamp_segment_count_it->second->_value_int.value());
      }

      auto opacity_it = data._tmx_object->_properties->_map.find("opacity");
      if (opacity_it != data._tmx_object->_properties->_map.end())
      {
         _opacity = static_cast<int32_t>(opacity_it->second->_value_int.value());
      }

      auto tension_it = data._tmx_object->_properties->_map.find("tension");
      if (tension_it != data._tmx_object->_properties->_map.end())
      {
         _config._tension = tension_it->second->_value_float.value();
      }

      auto dampening_it = data._tmx_object->_properties->_map.find("dampening");
      if (dampening_it != data._tmx_object->_properties->_map.end())
      {
         _config._dampening = dampening_it->second->_value_float.value();
      }

      auto spread_it = data._tmx_object->_properties->_map.find("spread");
      if (spread_it != data._tmx_object->_properties->_map.end())
      {
         _config._spread = spread_it->second->_value_float.value();
      }

      auto animation_speed_it = data._tmx_object->_properties->_map.find("animation_speed");
      if (animation_speed_it != data._tmx_object->_properties->_map.end())
      {
         _config._animation_speed = animation_speed_it->second->_value_float.value();
      }

      auto splash_factor_it = data._tmx_object->_properties->_map.find("splash_factor");
      if (splash_factor_it != data._tmx_object->_properties->_map.end())
      {
         _config._splash_factor = splash_factor_it->second->_value_float.value();
      }
   }

   for (auto i = 0; i < segment_count; i++)
   {
      _segments.push_back({});
   }

   // clamp corner edges if configured
   if (clamp_segment_count.has_value())
   {
      if (clamp_segment_count.value() * 2 < _segments.size())
      {
         const auto clamp_scale_increment = 1.0f / clamp_segment_count.value();
         auto scale = 0.0f;
         for (auto i = 0; i < clamp_segment_count; i++)
         {
            _segments[i]._clamp_scale = scale;
            _segments[_segments.size() - 1 - i]._clamp_scale = scale;
            scale += clamp_scale_increment;
         }
      }
      else
      {
         Log::Error() << "clamp_segment_count must be smaller than half of the segment_count";
      }
   }

   _vertices.setPrimitiveType(sf::PrimitiveType::TriangleStrip);
   _vertices.resize(segment_count * 2);

   // segment size - 1 has been chosen here to cover the entire range of the bounding box
   _segment_width = (_bounding_box.width / (_segments.size() - 1)) / _pixel_ratio.value_or(1.0f);
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
             static_cast<int32_t>((_bounding_box.height * 2.0f) / _pixel_ratio.value())
          ))
      {
         Log::Error() << "could not create render texture";
      }

      _render_texture.setSmooth(false);
      render_texture_sprite.setTexture(_render_texture.getTexture());
      render_texture_sprite.setPosition({_bounding_box.left, _bounding_box.top - _bounding_box.height});
      render_texture_sprite.scale(_pixel_ratio.value(), _pixel_ratio.value());
   }
}

void WaterSurface::updateEmitters(float elapsed_s)
{
   for (auto& emitter : _emitters)
   {
      emitter._elapsed_s += elapsed_s;

      if (emitter._elapsed_s > emitter._interval_s)
      {
         // reset emitter time
         emitter._elapsed_s = 0.0f;
         emitter._interval_s = frand(emitter._interval_min_s, emitter._interval_max_s);

         // generate splashes
         for (auto emitter_index = 0; emitter_index < emitter._count; emitter_index++)
         {
            const auto left_px = frand(emitter._x_from_px, emitter._x_to_px - emitter._width_px);
            const auto left_with_offset_px = emitter._x_offset_to_parent_px + left_px;
            const auto right_with_offset_px = emitter._x_offset_to_parent_px + left_px + emitter._width_px;

            const auto left_normalized = left_with_offset_px / _bounding_box.width;
            const auto right_normalized = right_with_offset_px / _bounding_box.width;

            const auto left_index = static_cast<int32_t>(left_normalized * _segments.size());
            const auto right_index = static_cast<int32_t>(right_normalized * _segments.size());

            for (auto index = left_index; index < right_index; index++)
            {
               splash(index, emitter._velocity);
            }
         }
      }
   }
}
