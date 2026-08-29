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

// each beat is a gaussian centred on its own phase, the second one weaker than the first, so the
// two together read as a lub-dub. a gaussian has no discontinuity anywhere, not even in its
// derivative, which neither an eased ramp nor a sine hump can say.
//
//   1.0 |     _
//       |    / \      _
//       |   /   \    / \
//   0.0 +__/     \__/   \______________
//       0        0.16                 1     phase
//
constexpr auto heartbeat_second_beat_phase = 0.16f;

float evaluateGaussianBeat(float phase, float center_phase, float width_phase)
{
   auto distance = std::abs(phase - center_phase);

   // the phase wraps, so a beat centred on 0 is still felt at the end of the previous period
   distance = std::min(distance, 1.0f - distance);

   const auto normalized_distance = distance / width_phase;

   return std::exp(-0.5f * normalized_distance * normalized_distance);
}

float evaluateHeartbeat(float phase, float second_beat_strength, float beat_width_phase)
{
   if (beat_width_phase <= 0.0f)
   {
      return 0.0f;
   }

   const auto first_beat = evaluateGaussianBeat(phase, 0.0f, beat_width_phase);
   const auto second_beat = evaluateGaussianBeat(phase, heartbeat_second_beat_phase, beat_width_phase) * second_beat_strength;

   // where the curves overlap they add up, which keeps the join between them smooth
   return std::min(first_beat + second_beat, 1.0f);
}

// disabling the ring cuts its power rather than hiding it. it stutters the way a tube does when
// the plug comes out, then whips shut onto the sword.
//
//   drawn    |###  ## ####################
//   hidden   |   ##  #
//            +----------------------------+
//            0                            1   progress
//
// the nearest point on the player rect hops between the rect's edges as he moves, and the
// physics resolution jitters it further, so the contact angle is eased rather than followed.
constexpr auto touch_angle_smoothing_s = 0.08f;

constexpr auto power_down_first_gap_start = 0.08f;
constexpr auto power_down_first_gap_end = 0.15f;
constexpr auto power_down_second_gap_start = 0.22f;
constexpr auto power_down_second_gap_end = 0.25f;

bool isPowerDownFlickerHidden(float progress)
{
   const auto in_first_gap = (progress >= power_down_first_gap_start && progress < power_down_first_gap_end);
   const auto in_second_gap = (progress >= power_down_second_gap_start && progress < power_down_second_gap_end);

   return in_first_gap || in_second_gap;
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
   _power_down_s = ValueReader::readValue<float>("power_down_s", map).value_or(_power_down_s);
}

#ifdef DECEPTUS_VRSFML
void RingShaderLayer::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   draw(target, normal, {});
}

void RingShaderLayer::draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states)
{
   if (!_shader.isLoaded() || _power_down_progress >= 1.0f || isPowerDownFlickerHidden(_power_down_progress))
   {
      return;
   }

   // NOTE: the ring used to render far too large on WASM for the same ring_scale. cause was the
   // GL_ES branch of ring.vert scaling the screen uv by sf_u_invTextureSize (which reflects an
   // unrelated bound texture's size); ring.vert now passes the raw 0..1 texcoord, matching desktop.
   _shader.setUniform("u_ring_scale", currentRingScale());
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
   if (_power_down_progress >= 1.0f || isPowerDownFlickerHidden(_power_down_progress))
   {
      return;
   }

   if (_has_u_ring_scale)
   {
      _shader.setUniform("u_ring_scale", currentRingScale());
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

   if (!isEnabled())
   {
      const auto elapsed_s = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - _disable_time).count();
      _power_down_progress = (_power_down_s > 0.0f) ? std::min(elapsed_s / _power_down_s, 1.0f) : 1.0f;

      // a ring that has lost its power neither beats nor answers the player
      _heartbeat_pulse = 0.0f;
      _touch_intensity = 0.0f;
      _touched = false;
      return;
   }

   _power_down_progress = 0.0f;

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

float RingShaderLayer::currentRingScale() const
{
   const auto beat_scale = std::lerp(1.0f, _heartbeat_scale, _heartbeat_pulse);

   // once the power is cut the band whips shut onto the sword
   const auto power_down_scale = 1.0f - Easings::easeInCubic<float>(_power_down_progress);

   return _ring_scale * beat_scale * power_down_scale;
}

void RingShaderLayer::updateTouch(const sf::Time& dt)
{
   const auto& player_rect = PlayerRegistry::getFirst()->getPixelRectFloat();
   const auto center = sf::Vector2f{_position.x + _size.x * 0.5f, _position.y + _size.y * 0.5f};

   // the band sits where circularEffect crosses zero, i.e. at a length of 14/12 in ring space.
   // deliberately the resting radius, without the beat: a band that moves with the heartbeat would
   // slide past a player standing at its edge, and since contact restarts the beat that feeds back
   // into itself and the ring shakes.
   constexpr auto band_length = 14.0f / 12.0f;
   const auto band_radius_px = band_length * _ring_scale * _size.x;

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
      const auto target_angle = std::atan2(-nearest.y, nearest.x);

      if (_touched)
      {
         // ease around the ring the short way, so crossing the wrap point does not spin the dent
         auto angle_delta = target_angle - _touch_angle;
         angle_delta = std::remainder(angle_delta, 2.0f * std::numbers::pi_v<float>);
         _touch_angle += angle_delta * std::min(dt.asSeconds() / touch_angle_smoothing_s, 1.0f);
      }
      else
      {
         // first frame of contact, the dent belongs where he actually is
         _touch_angle = target_angle;

         // and the ward beats the moment it is touched
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
