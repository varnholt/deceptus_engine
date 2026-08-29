#include "ringshaderlayer.h"

#include "framework/easings/easings.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "game/io/valuereader.h"
#include "game/player/playerregistry.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#ifndef DECEPTUS_VRSFML
#include <fstream>
#include <sstream>
#endif

namespace
{
struct RingShaderLayerRegister
{
   RingShaderLayerRegister()
   {
      ShaderLayer::registerCustomization("ring", [](GameNode* parent) { return std::make_shared<RingShaderLayer>(parent); });
   }
};

static RingShaderLayerRegister reg;

// the flash swells over its duration instead of jumping to full on the first frame and fading out
// linearly. the three phases are fixed proportions of the duration handed to flash(), so callers
// still pass a single value.
//
//   1.00 |     ,-----.
//        |    /       `--.
//   0.75 |   /            `---.
//        |  /                  `-------.
//   0.00 | /                            `----------
//        +--+---------+--------------------------+
//         attack   sustain         release
//
constexpr auto flash_attack_ratio = 0.09f;
constexpr auto flash_sustain_ratio = 0.32f;
constexpr auto flash_plateau_intensity = 0.75f;

// each beat is a single sine hump. the second one starts behind the first and is slightly weaker,
// so the two overlap into a lub-dub. a hump leaves and returns to zero with no corner at either
// end, which an eased attack/release pair does not.
//
//   1.0 |    .-.
//       |   /   \   .-.
//       |  /     \ /   \
//   0.0 +-'       '     '----------------
//       0        0.16                   1     phase
//
constexpr auto heartbeat_second_beat_phase = 0.16f;

float evaluateSineBeat(float phase, float start_phase, float width_phase)
{
   const auto local_phase = phase - start_phase;

   if (local_phase < 0.0f || local_phase > width_phase)
   {
      return 0.0f;
   }

   return std::sin(std::numbers::pi_v<float> * local_phase / width_phase);
}

float evaluateHeartbeat(float phase, float second_beat_strength, float beat_width_phase)
{
   if (beat_width_phase <= 0.0f)
   {
      return 0.0f;
   }

   const auto first_beat = evaluateSineBeat(phase, 0.0f, beat_width_phase);
   const auto second_beat = evaluateSineBeat(phase, heartbeat_second_beat_phase, beat_width_phase) * second_beat_strength;

   // where the humps overlap they add up, which keeps the join between them smooth
   return std::min(first_beat + second_beat, 1.0f);
}

float evaluateFlashEnvelope(float elapsed_s, float duration_s)
{
   const auto attack_duration_s = duration_s * flash_attack_ratio;
   const auto sustain_duration_s = duration_s * flash_sustain_ratio;
   const auto release_duration_s = duration_s - attack_duration_s - sustain_duration_s;

   if (elapsed_s < attack_duration_s)
   {
      return Easings::easeOutCubic<float>(elapsed_s / attack_duration_s);
   }

   if (elapsed_s < attack_duration_s + sustain_duration_s)
   {
      const auto normalized = (elapsed_s - attack_duration_s) / sustain_duration_s;
      return std::lerp(1.0f, flash_plateau_intensity, Easings::easeInOutCubic<float>(normalized));
   }

   const auto normalized = std::min((elapsed_s - attack_duration_s - sustain_duration_s) / release_duration_s, 1.0f);
   return flash_plateau_intensity * (1.0f - Easings::easeInOutCubic<float>(normalized));
}
}  // namespace

RingShaderLayer::RingShaderLayer(GameNode* parent) : ShaderLayer(parent)
{
}

#ifndef DECEPTUS_VRSFML
void RingShaderLayer::checkUniforms(const std::string& shader_path)
{
   ShaderLayer::checkUniforms(shader_path);

   std::ifstream file(shader_path);
   if (!file.is_open())
   {
      return;
   }

   std::stringstream buffer;
   buffer << file.rdbuf();
   const auto shader_source = buffer.str();

   _has_u_ring_scale = shader_source.find("u_ring_scale;") != std::string::npos;
   _has_u_pixel_size = shader_source.find("u_pixel_size;") != std::string::npos;
   _has_u_flash_color = shader_source.find("u_flash_color;") != std::string::npos;
   _has_u_flash_intensity = shader_source.find("u_flash_intensity;") != std::string::npos;
   _has_u_touch = shader_source.find("u_touch_intensity;") != std::string::npos;
}
#endif

void RingShaderLayer::readCustomProperties(const GameDeserializeData& data)
{
   const auto& map = data._tmx_object->_properties->_map;
   _ring_scale = ValueReader::readValue<float>("ring_scale", map).value_or(_ring_scale);
   _pixel_size = ValueReader::readValue<float>("pixel_size", map).value_or(_pixel_size);
   _heartbeat_period_s = ValueReader::readValue<float>("heartbeat_period_s", map).value_or(_heartbeat_period_s);
   _heartbeat_scale = ValueReader::readValue<float>("heartbeat_scale", map).value_or(_heartbeat_scale);
   _heartbeat_second_beat = ValueReader::readValue<float>("heartbeat_second_beat", map).value_or(_heartbeat_second_beat);
   _heartbeat_turbulence = ValueReader::readValue<float>("heartbeat_turbulence", map).value_or(_heartbeat_turbulence);
   _heartbeat_beat_width = ValueReader::readValue<float>("heartbeat_beat_width", map).value_or(_heartbeat_beat_width);
   _touch_depth = ValueReader::readValue<float>("touch_depth", map).value_or(_touch_depth);
   _touch_width = ValueReader::readValue<float>("touch_width", map).value_or(_touch_width);
   _touch_release_s = ValueReader::readValue<float>("touch_release_s", map).value_or(_touch_release_s);
}

#ifdef DECEPTUS_VRSFML
void RingShaderLayer::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   draw(target, normal, {});
}

void RingShaderLayer::draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states)
{
   if (!_shader.isLoaded())
   {
      return;
   }

   // NOTE: the ring used to render far too large on WASM for the same ring_scale. cause was the
   // GL_ES branch of ring.vert scaling the screen uv by sf_u_invTextureSize (which reflects an
   // unrelated bound texture's size); ring.vert now passes the raw 0..1 texcoord, matching desktop.
   _shader.setUniform("u_ring_scale", _ring_scale * std::lerp(1.0f, _heartbeat_scale, _heartbeat_pulse));
   _shader.setUniform("u_pixel_size", _pixel_size);
   _shader.setUniform("u_flash_color", _flash_color);
   _shader.setUniform("u_flash_intensity", _flash_intensity);
   _shader.setUniform("u_touch_angle", _touch_angle);
   _shader.setUniform("u_touch_intensity", _touch_intensity);
   _shader.setUniform("u_touch_width", _touch_width);

   ShaderLayer::draw(target, normal, states);
}
#else
void RingShaderLayer::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   if (_has_u_ring_scale)
   {
      _shader.setUniform("u_ring_scale", _ring_scale * std::lerp(1.0f, _heartbeat_scale, _heartbeat_pulse));
   }

   if (_has_u_pixel_size)
   {
      _shader.setUniform("u_pixel_size", _pixel_size);
   }

   if (_has_u_flash_color)
   {
      _shader.setUniform("u_flash_color", _flash_color);
   }

   if (_has_u_flash_intensity)
   {
      _shader.setUniform("u_flash_intensity", _flash_intensity);
   }

   if (_has_u_touch)
   {
      _shader.setUniform("u_touch_angle", _touch_angle);
      _shader.setUniform("u_touch_intensity", _touch_intensity);
      _shader.setUniform("u_touch_width", _touch_width);
   }

   ShaderLayer::draw(target, normal);
}
#endif

void RingShaderLayer::update(const sf::Time& dt)
{
   ShaderLayer::update(dt);

   updateTouch(dt);

   if (_heartbeat_period_s > 0.0f)
   {
      _heartbeat_elapsed_s = std::fmod(_heartbeat_elapsed_s + dt.asSeconds(), _heartbeat_period_s);
      _heartbeat_pulse = evaluateHeartbeat(_heartbeat_elapsed_s / _heartbeat_period_s, _heartbeat_second_beat, _heartbeat_beat_width);

      // the beat drives the turbulence too. advancing the time offset speeds up the churn without
      // the phase jump a scaled u_time would produce.
      _time_offset += dt.asSeconds() * _heartbeat_pulse * _heartbeat_turbulence;
   }

   if (_flash_duration > 0.0f)
   {
      _flash_elapsed += dt.asSeconds();
      _flash_intensity = evaluateFlashEnvelope(_flash_elapsed, _flash_duration);
      if (_flash_elapsed >= _flash_duration)
      {
         _flash_duration = 0.0f;
         _flash_intensity = 0.0f;
      }
   }
}

void RingShaderLayer::updateTouch(const sf::Time& dt)
{
   const auto& player_rect = PlayerRegistry::getFirst()->getPixelRectFloat();
   const auto center = sf::Vector2f{_position.x + _size.x * 0.5f, _position.y + _size.y * 0.5f};

   // the band sits where circularEffect crosses zero, i.e. at a length of 14/12 in ring space
   constexpr auto band_length = 14.0f / 12.0f;
   const auto band_radius_px = band_length * _ring_scale * std::lerp(1.0f, _heartbeat_scale, _heartbeat_pulse) * _size.x;

   // nearest and farthest corner of the player rect decide whether the circle cuts through it
   const auto clamped_x = std::clamp(center.x, player_rect.position.x, player_rect.position.x + player_rect.size.x);
   const auto clamped_y = std::clamp(center.y, player_rect.position.y, player_rect.position.y + player_rect.size.y);
   const auto nearest = sf::Vector2f{clamped_x - center.x, clamped_y - center.y};
   const auto nearest_distance = std::hypot(nearest.x, nearest.y);

   const auto farthest_x =
      std::max(std::abs(player_rect.position.x - center.x), std::abs(player_rect.position.x + player_rect.size.x - center.x));
   const auto farthest_y =
      std::max(std::abs(player_rect.position.y - center.y), std::abs(player_rect.position.y + player_rect.size.y - center.y));
   const auto farthest_distance = std::hypot(farthest_x, farthest_y);

   const auto touching = (nearest_distance <= band_radius_px && farthest_distance >= band_radius_px);

   if (touching)
   {
      // the shader works in texture space, where y points up, so the world offset is flipped
      _touch_angle = std::atan2(-nearest.y, nearest.x);

      // the ward beats the moment it is touched
      if (!_touched)
      {
         _heartbeat_elapsed_s = 0.0f;
      }

      _touch_intensity = _touch_depth;
   }
   else if (_touch_intensity > 0.0f && _touch_release_s > 0.0f)
   {
      _touch_intensity = std::max(_touch_intensity - _touch_depth * dt.asSeconds() / _touch_release_s, 0.0f);
   }

   _touched = touching;
}

void RingShaderLayer::setEnabled(bool enabled)
{
   if (!enabled)
   {
      _disable_time = std::chrono::high_resolution_clock::now();
   }

   ShaderLayer::setEnabled(enabled);
}

void RingShaderLayer::flash(float red, float green, float blue, float duration_s)
{
   _flash_color = sf::Glsl::Vec3{red, green, blue};
   _flash_duration = duration_s;
   _flash_elapsed = 0.0f;
}
