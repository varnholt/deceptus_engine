#include "ringshaderlayer.h"

#include "framework/easings/easings.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "game/io/valuereader.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
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

// the ring is a customization inside the shader_quads layer, so it shares the layer with the plain
// quads but needs its own type: tiled gets a separate template and property list, and a fog quad
// is not offered a heartbeat. customization is a template_value because inserting the template has
// to produce a ring, not a blank quad.
static constexpr std::array ring_shader_quad_properties{
   PropertyInfo{
      .name = "customization",
      .type = "string",
      .default_value = std::string_view{""},
      .required = true,
      .template_value = std::string_view{"ring"}
   },
   PropertyInfo{
      .name = "fragment_shader",
      .type = "string",
      .default_value = std::string_view{""},
      .required = true,
      .template_value = std::string_view{"data/shaders/ring.frag"}
   },
   PropertyInfo{
      .name = "vertex_shader",
      .type = "string",
      .default_value = std::string_view{""},
      .template_value = std::string_view{"data/shaders/ring.vert"}
   },
   PropertyInfo{
      .name = "texture",
      .type = "string",
      .default_value = std::string_view{""},
      .template_value = std::string_view{"data/effects/grainy.png"}
   },
   PropertyInfo{.name = "smooth_texture", .type = "bool", .default_value = false, .template_value = true},
   PropertyInfo{.name = "z", .type = "int", .default_value = int32_t{20}},

   // the band sits at 14/12 in ring space, so its radius on screen is
   // (14/12) * ring_scale * object width. the object has to be wide enough to hold the release.
   PropertyInfo{.name = "ring_scale", .type = "float", .default_value = 1.0f / 3.0f, .template_value = 0.052083f},
   PropertyInfo{.name = "pixel_size", .type = "float", .default_value = 1.0f},

   PropertyInfo{.name = "heartbeat_period_s", .type = "float", .default_value = 2.0f},
   PropertyInfo{.name = "heartbeat_scale", .type = "float", .default_value = 1.1f},
   PropertyInfo{.name = "heartbeat_second_beat", .type = "float", .default_value = 0.7f},
   PropertyInfo{.name = "heartbeat_beat_width", .type = "float", .default_value = 0.035f},
   PropertyInfo{.name = "heartbeat_turbulence", .type = "float", .default_value = 1.0f},

   PropertyInfo{.name = "touch_depth", .type = "float", .default_value = 0.22f},
   PropertyInfo{.name = "touch_width", .type = "float", .default_value = 0.55f},
   PropertyInfo{.name = "touch_release_s", .type = "float", .default_value = 0.35f},

   PropertyInfo{.name = "push_px", .type = "float", .default_value = 10.0f},
   PropertyInfo{.name = "push_release_s", .type = "float", .default_value = 0.6f},
   PropertyInfo{.name = "hit_attack_s", .type = "float", .default_value = 0.09f},

   PropertyInfo{.name = "power_down_s", .type = "float", .default_value = 1.2f},
};

static constexpr MechanismSchema ring_shader_quad_schema{
   .type_name = "RingShaderQuad",
   .layer_name = "shader_quads",
   .default_width = 768,
   .default_height = 768,
   .properties = ring_shader_quad_properties,
};

const auto registered_ring_shader_quad = []
{
   GameMechanismDeserializerRegistry::instance().registerSchema(ring_shader_quad_schema);
   return true;
}();

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

// a released ring does not collapse onto the sword, it lets go outwards: the band accelerates
// away across the whole screen and thins out as it goes, so the last of it passes the player
// rather than disappearing in front of him. nothing is ever cut.
//
//   scale    |                 _--
//            |           __--''
//            |'''-----'''
//   dissolve |               __--''
//            |''''''''---.--'
//            +------------------------+
//            0                        1   progress
//
// the dissolve has to be finished before the band's glow reaches the quad's edge or it would be
// cut off in a straight line, which is what ties the ceiling below to the size of the tmx object.
constexpr auto power_down_scale_ceiling = 10.0f;
constexpr auto power_down_dissolve_ceiling = 1.05f;
constexpr auto power_down_dissolve_end = 0.7f;

// the nearest point on the player rect hops between the rect's edges as he moves, and the
// physics resolution jitters it further, so the contact angle is eased rather than followed.
constexpr auto touch_angle_smoothing_s = 0.08f;

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
   _has_u_push = shader_source.find("u_push;") != std::string::npos;
   _has_u_dissolve = shader_source.find("u_dissolve;") != std::string::npos;
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
   _push_px = ValueReader::readValue<float>("push_px", map).value_or(_push_px);
   _push_release_s = ValueReader::readValue<float>("push_release_s", map).value_or(_push_release_s);
   _hit_attack_s = ValueReader::readValue<float>("hit_attack_s", map).value_or(_hit_attack_s);
}

#ifdef DECEPTUS_VRSFML
void RingShaderLayer::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   draw(target, normal, {});
}

void RingShaderLayer::draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states)
{
   if (!_shader.isLoaded() || _power_down_progress >= 1.0f)
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
   _shader.setUniform("u_push", sf::Glsl::Vec2{_push_offset_px.x / _size.x, _push_offset_px.y / _size.y});
   _shader.setUniform("u_dissolve", currentDissolve());

   ShaderLayer::draw(target, normal, states);
}
#else
void RingShaderLayer::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   if (_power_down_progress >= 1.0f)
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

   if (_has_u_push)
   {
      _shader.setUniform("u_push", sf::Glsl::Vec2{_push_offset_px.x / _size.x, _push_offset_px.y / _size.y});
   }

   if (_has_u_dissolve)
   {
      _shader.setUniform("u_dissolve", currentDissolve());
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
      _push_target_px = {};
      _push_offset_px = {};
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

float RingShaderLayer::currentDissolve() const
{
   const auto dissolve_progress = std::min(_power_down_progress / power_down_dissolve_end, 1.0f);

   return power_down_dissolve_ceiling * Easings::easeInCubic<float>(dissolve_progress);
}

float RingShaderLayer::currentRingScale() const
{
   const auto beat_scale = std::lerp(1.0f, _heartbeat_scale, _heartbeat_pulse);

   // quadratic rather than cubic: it still builds, but it creeps for a moment first instead of
   // sitting still and then bolting
   const auto power_down_scale = std::lerp(1.0f, power_down_scale_ceiling, Easings::easeInQuad<float>(_power_down_progress));

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

         // the whole ring gives ground, away from the side it was hit from. this only aims the
         // recoil, the ring travels there over hit_attack_s. set rather than accumulated, so
         // leaning on it cannot walk the ring off the sword.
         _push_target_px = {-std::cos(target_angle) * _push_px, -std::sin(target_angle) * _push_px};
      }

      // eased in rather than set, or the dent appears fully formed in a single frame
      const auto attack = (_hit_attack_s > 0.0f) ? std::min(dt.asSeconds() / _hit_attack_s, 1.0f) : 1.0f;
      _touch_intensity += (_touch_depth - _touch_intensity) * attack;
   }
   else if (_touch_intensity > 0.0f && _touch_release_s > 0.0f)
   {
      _touch_intensity = std::max(_touch_intensity - _touch_depth * dt.asSeconds() / _touch_release_s, 0.0f);
   }

   _touched = touching;

   // the aim point falls back toward the sword whether or not he is still against it, and the
   // ring chases the aim point rather than being placed on it. the chase is what gives the recoil
   // some travel instead of a jump; the falling aim point is what brings it home.
   //
   //   push |    ,--.
   //        |   /    `--.
   //        |  /         `-----.
   //      0 +-'                 `--------
   //        |<-->| hit_attack_s
   //
   if (_push_release_s > 0.0f)
   {
      const auto recovered = std::max(1.0f - dt.asSeconds() / _push_release_s, 0.0f);
      _push_target_px = {_push_target_px.x * recovered, _push_target_px.y * recovered};
   }

   const auto chase = (_hit_attack_s > 0.0f) ? std::min(dt.asSeconds() / _hit_attack_s, 1.0f) : 1.0f;
   _push_offset_px.x += (_push_target_px.x - _push_offset_px.x) * chase;
   _push_offset_px.y += (_push_target_px.y - _push_offset_px.y) * chase;
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
