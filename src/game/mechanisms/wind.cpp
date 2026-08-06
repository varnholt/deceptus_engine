#include "wind.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <limits>
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"
#include "framework/tools/log.h"
#include "framework/tools/sfmlcompat.h"
#include "game/audio/audio.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/playerregistry.h"

namespace
{
static constexpr float default_wind_direction_x = 0.0f;
static constexpr float default_wind_direction_y = 0.0f;
static constexpr float default_wind_strength = 0.0f;
static constexpr int32_t default_wind_z = 20;
static constexpr float default_sound_volume = 1.0f;
static constexpr float default_sound_radius_near_px = 200.0f;
static constexpr float default_sound_radius_far_px = 800.0f;
static constexpr float default_sound_strength_influence = 0.0f;
static constexpr std::string_view default_leaf_texture = "data/sprites/leaves_fall.png";
static constexpr int32_t default_leaf_count = 0;
static constexpr int32_t default_leaf_frame_size_px = 16;
static constexpr float default_leaf_velocity_px_s = 60.0f;
static constexpr float default_leaf_jitter_amount = 0.35f;
static constexpr float default_leaf_jitter_frequency_hz = 0.8f;
static constexpr float default_leaf_animation_speed = 8.0f;
static constexpr float default_leaf_scale = 1.0f;
static constexpr float default_leaf_alpha = 1.0f;
static constexpr float default_leaf_sound_influence = 0.0f;

static constexpr auto two_pi = 6.283185307179586f;

// a leaf that cannot travel anywhere - because no direction was configured - is recycled after a
// random while instead of hanging around forever
static constexpr auto leaf_lifetime_min_s = 4.0f;
static constexpr auto leaf_lifetime_max_s = 12.0f;

// fade duration at both ends of a leaf's life so it does not pop in and out at the area's borders
static constexpr auto leaf_fade_duration_s = 0.75f;

// leaves all travelling at exactly the same speed read as a texture rather than as single leaves
static constexpr auto leaf_speed_factor_min = 0.6f;
static constexpr auto leaf_speed_factor_max = 1.4f;
static constexpr auto leaf_jitter_frequency_factor_min = 0.75f;
static constexpr auto leaf_jitter_frequency_factor_max = 1.25f;

static constexpr std::array wind_properties{
   PropertyInfo{.name = "direction_x", .type = "float", .default_value = default_wind_direction_x},
   PropertyInfo{.name = "direction_y", .type = "float", .default_value = default_wind_direction_y},
   PropertyInfo{.name = "strength", .type = "float", .default_value = default_wind_strength},
   PropertyInfo{.name = "z", .type = "int", .default_value = default_wind_z},
   PropertyInfo{.name = "sounds", .type = "string", .default_value = std::string_view{}},
   PropertyInfo{.name = "sound_volume", .type = "float", .default_value = default_sound_volume},
   PropertyInfo{.name = "sound_radius_near_px", .type = "float", .default_value = default_sound_radius_near_px},
   PropertyInfo{.name = "sound_radius_far_px", .type = "float", .default_value = default_sound_radius_far_px},
   PropertyInfo{.name = "sound_strength_influence", .type = "float", .default_value = default_sound_strength_influence},
   PropertyInfo{.name = "leaf_count", .type = "int", .default_value = default_leaf_count},
   PropertyInfo{.name = "leaf_texture", .type = "string", .default_value = default_leaf_texture},
   PropertyInfo{.name = "leaf_frame_size_px", .type = "int", .default_value = default_leaf_frame_size_px},
   PropertyInfo{.name = "leaf_velocity_px_s", .type = "float", .default_value = default_leaf_velocity_px_s},
   PropertyInfo{.name = "leaf_jitter_amount", .type = "float", .default_value = default_leaf_jitter_amount},
   PropertyInfo{.name = "leaf_jitter_frequency_hz", .type = "float", .default_value = default_leaf_jitter_frequency_hz},
   PropertyInfo{.name = "leaf_animation_speed", .type = "float", .default_value = default_leaf_animation_speed},
   PropertyInfo{.name = "leaf_scale_min", .type = "float", .default_value = default_leaf_scale},
   PropertyInfo{.name = "leaf_scale_max", .type = "float", .default_value = default_leaf_scale},
   PropertyInfo{.name = "leaf_alpha", .type = "float", .default_value = default_leaf_alpha},
   PropertyInfo{.name = "leaf_sound_influence", .type = "float", .default_value = default_leaf_sound_influence},
};
static constexpr MechanismSchema wind_schema{
   .type_name = "Wind",
   .layer_name = "wind",
   .default_width = 192,
   .default_height = 192,
   .properties = wind_properties,
};
const auto registered_wind = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.registerSchema(wind_schema);
   registry.mapGroupToLayer("Wind", "wind");

   registry.registerLayerName(
      "wind",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto instance = Wind::deserialize(parent, data);
         mechanisms["wind"]->push_back(instance);
      }
   );

   registry.registerObjectGroup(
      "Wind",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto instance = Wind::deserialize(parent, data);
         mechanisms["wind"]->push_back(instance);
      }
   );

   return true;
}();

float randomFloat(float min_value, float max_value)
{
   const auto normalized = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
   return min_value + normalized * (max_value - min_value);
}

// read "sample_1.ogg;sample_2.ogg;sample_3.ogg"
std::vector<std::string> parseSoundList(const std::string& sounds)
{
   std::vector<std::string> result;

   for (const auto& sound : TmxTools::split(sounds, ';'))
   {
      if (!sound.empty())
      {
         result.push_back(sound);
      }
   }

   return result;
}

/// \brief returns how far a point inside a rectangle can travel along a direction before it leaves it.
/// \param rect rectangle the point travels through.
/// \param position_px point inside the rectangle.
/// \param direction unit vector the point travels along.
/// \return distance in pixels, or 0 when the direction is a zero vector.
float distanceToBorderPx(const sf::FloatRect& rect, const sf::Vector2f& position_px, const sf::Vector2f& direction)
{
   auto distance_px = std::numeric_limits<float>::max();

   if (direction.x > 0.0f)
   {
      distance_px = std::min(distance_px, (rect.position.x + rect.size.x - position_px.x) / direction.x);
   }
   else if (direction.x < 0.0f)
   {
      distance_px = std::min(distance_px, (position_px.x - rect.position.x) / -direction.x);
   }

   if (direction.y > 0.0f)
   {
      distance_px = std::min(distance_px, (rect.position.y + rect.size.y - position_px.y) / direction.y);
   }
   else if (direction.y < 0.0f)
   {
      distance_px = std::min(distance_px, (position_px.y - rect.position.y) / -direction.y);
   }

   if (distance_px == std::numeric_limits<float>::max())
   {
      return 0.0f;
   }

   return std::max(0.0f, distance_px);
}
}  // namespace

Wind::Wind(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Wind).name());
   setZ(default_wind_z);
}

Wind::~Wind()
{
   stopPlaying();
}

std::string_view Wind::objectName() const
{
   return "Wind";
}

std::shared_ptr<Wind> Wind::deserialize(GameNode* parent, const GameDeserializeData& data)
{
   auto wind = std::make_shared<Wind>(parent);
   wind->setObjectId(data._tmx_object->_name);

   wind->_area = {{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   auto sound_volume = default_sound_volume;
   auto sound_radius_near_px = default_sound_radius_near_px;
   auto sound_radius_far_px = default_sound_radius_far_px;

   if (data._tmx_object->_properties)
   {
      const auto& props = data._tmx_object->_properties->_map;

      auto direction = sf::Vector2f{
         ValueReader::readValue<float>("direction_x", props).value_or(default_wind_direction_x),
         ValueReader::readValue<float>("direction_y", props).value_or(default_wind_direction_y)
      };

      // the direction only says where the wind blows, how hard it blows is 'strength'. normalizing here
      // means a level can be re-tuned through a single property.
      const auto length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
      if (length > 0.0f)
      {
         direction /= length;
      }

      wind->_direction = direction;
      wind->_direction_screen = {direction.x, -direction.y};

      // zones authored before 'strength' existed used the direction vector's length as the force. falling
      // back to that length keeps them applying the exact same force without having to touch the level.
      wind->_strength = ValueReader::readValue<float>("strength", props).value_or(length);

      wind->setZ(ValueReader::readValue<int32_t>("z", props).value_or(default_wind_z));

      // audio
      const auto sounds = ValueReader::readValue<std::string>("sounds", props);
      if (sounds.has_value())
      {
         wind->_sounds = parseSoundList(sounds.value());
         for (const auto& sound : wind->_sounds)
         {
            Audio::getInstance().addSample(sound);
         }
      }

      sound_volume = ValueReader::readValue<float>("sound_volume", props).value_or(default_sound_volume);
      sound_radius_near_px = ValueReader::readValue<float>("sound_radius_near_px", props).value_or(default_sound_radius_near_px);
      sound_radius_far_px = ValueReader::readValue<float>("sound_radius_far_px", props).value_or(default_sound_radius_far_px);
      wind->_sound_strength_influence = std::clamp(
         ValueReader::readValue<float>("sound_strength_influence", props).value_or(default_sound_strength_influence), 0.0f, 1.0f
      );

      // leaves
      auto& leaf_settings = wind->_leaf_settings;
      leaf_settings._texture_path = ValueReader::readValue<std::string>("leaf_texture", props).value_or(std::string{default_leaf_texture});
      leaf_settings._count = ValueReader::readValue<int32_t>("leaf_count", props).value_or(default_leaf_count);
      leaf_settings._frame_size_px = ValueReader::readValue<int32_t>("leaf_frame_size_px", props).value_or(default_leaf_frame_size_px);
      leaf_settings._velocity_px_s = ValueReader::readValue<float>("leaf_velocity_px_s", props).value_or(default_leaf_velocity_px_s);
      leaf_settings._jitter_amount = ValueReader::readValue<float>("leaf_jitter_amount", props).value_or(default_leaf_jitter_amount);
      leaf_settings._jitter_frequency_hz =
         ValueReader::readValue<float>("leaf_jitter_frequency_hz", props).value_or(default_leaf_jitter_frequency_hz);
      leaf_settings._animation_speed = ValueReader::readValue<float>("leaf_animation_speed", props).value_or(default_leaf_animation_speed);
      leaf_settings._scale_min = ValueReader::readValue<float>("leaf_scale_min", props).value_or(default_leaf_scale);
      leaf_settings._scale_max = ValueReader::readValue<float>("leaf_scale_max", props).value_or(default_leaf_scale);
      leaf_settings._alpha = ValueReader::readValue<float>("leaf_alpha", props).value_or(default_leaf_alpha);
      leaf_settings._sound_influence =
         std::clamp(ValueReader::readValue<float>("leaf_sound_influence", props).value_or(default_leaf_sound_influence), 0.0f, 1.0f);
   }

   if (!wind->_sounds.empty())
   {
      wind->_has_audio = true;
      wind->_reference_volume = sound_volume;
      wind->_audio_update_data._range = AudioRange{sound_radius_far_px, 0.0f, sound_radius_near_px, sound_volume};
   }

   wind->initializeLeaves();
   wind->addChunks(wind->_area);
   return wind;
}

void Wind::initializeLeaves()
{
   if (_leaf_settings._count <= 0)
   {
      return;
   }

   if (_leaf_settings._frame_size_px <= 0)
   {
      Log::Error() << "leaf_frame_size_px must be greater than 0: " << _object_id;
      return;
   }

   _leaf_texture = TexturePool::getInstance().get(_leaf_settings._texture_path);
   if (!_leaf_texture)
   {
      Log::Error() << "unable to load leaf texture: " << _leaf_settings._texture_path;
      return;
   }

   _leaf_frame_count = std::max(1, static_cast<int32_t>(_leaf_texture->getSize().x) / _leaf_settings._frame_size_px);

   for (auto index = 0; index < _leaf_settings._count; index++)
   {
      Leaf leaf;
#ifdef __EMSCRIPTEN__
      leaf._sprite = std::make_unique<sf::Sprite>();
#else
      leaf._sprite = std::make_unique<sf::Sprite>(*_leaf_texture);
#endif

      respawnLeaf(leaf, true);
      _leaves.push_back(std::move(leaf));
   }
}

void Wind::respawnLeaf(Leaf& leaf, bool inside_area) const
{
   auto position_px = sf::Vector2f{
      randomFloat(_area.position.x, _area.position.x + _area.size.x), randomFloat(_area.position.y, _area.position.y + _area.size.y)
   };

   if (!inside_area)
   {
      // move the leaf back to the border the wind blows in from so it travels across the whole area
      const auto upwind = sf::Vector2f{-_direction_screen.x, -_direction_screen.y};
      position_px += upwind * distanceToBorderPx(_area, position_px, upwind);
   }

   leaf._position_px = position_px;
   leaf._age_s = 0.0f;
   leaf._jitter_phase = randomFloat(0.0f, two_pi);
   leaf._jitter_frequency_hz =
      _leaf_settings._jitter_frequency_hz * randomFloat(leaf_jitter_frequency_factor_min, leaf_jitter_frequency_factor_max);
   leaf._speed_factor = randomFloat(leaf_speed_factor_min, leaf_speed_factor_max);
   leaf._animation_offset_s = randomFloat(0.0f, leaf_lifetime_max_s);
   leaf._scale = randomFloat(_leaf_settings._scale_min, _leaf_settings._scale_max);
   leaf._frame = -1;

   const auto frame_size_px = static_cast<float>(_leaf_settings._frame_size_px);
   sfcompat::setOrigin(*leaf._sprite, {frame_size_px * 0.5f, frame_size_px * 0.5f});

   // the leaves are mirrored instead of rotated so they stay aligned to the pixel grid
   const auto flip_horizontally = _direction_screen.x < 0.0f;
   sfcompat::setScale(*leaf._sprite, {flip_horizontally ? -leaf._scale : leaf._scale, leaf._scale});

   // tying the lifetime to the time it takes to cross the area lets the fade at both ends of the
   // leaf's life line up with the area's borders
   const auto travel_px = distanceToBorderPx(_area, position_px, _direction_screen);
   const auto speed_px_s = _leaf_settings._velocity_px_s * leaf._speed_factor;
   if (travel_px > 0.0f && speed_px_s > 0.0f)
   {
      leaf._lifetime_s = std::max(2.0f * leaf_fade_duration_s, travel_px / speed_px_s);
   }
   else
   {
      leaf._lifetime_s = randomFloat(leaf_lifetime_min_s, leaf_lifetime_max_s);
   }
}

void Wind::update(const sf::Time& dt)
{
   if (!_enabled)
   {
      // a disabled zone must not keep a looping sample alive; playback resumes once it is enabled again
      stopPlaying();
      applyLoudness(1.0f);
      return;
   }

   updateSound(dt);
   updateLeaves(dt);

   if (_strength <= 0.0f)
   {
      return;
   }

   const auto player = PlayerRegistry::getFirst();
   const auto& player_rect = player->getPixelRectFloat();
   if (!sfcompat::findIntersection(_area, player_rect).has_value())
   {
      return;
   }

   const auto force = _direction * _strength * _strength_factor;

   auto* body = player->getBody();
   body->ApplyForceToCenter(b2Vec2(force.x, -force.y), true);
}

void Wind::applyLoudness(float loudness)
{
   // the force and the leaves read the same loudness through their own influence, so a zone can push
   // the player constantly while its leaves gust, or the other way round
   _strength_factor = std::lerp(1.0f, loudness, _sound_strength_influence);
   _leaf_factor = std::lerp(1.0f, loudness, _leaf_settings._sound_influence);
}

void Wind::updateSound(const sf::Time& dt)
{
   if (_sounds.empty())
   {
      return;
   }

   if (!_audio_enabled)
   {
      applyLoudness(1.0f);
      return;
   }

   if (!_sound_thread_id.has_value())
   {
      playNextSound();
   }
   else if (_sounds.size() > 1 && _current_sound_duration_s > 0.0f)
   {
      // a single sample is looped, a list of samples keeps being re-randomized
      _elapsed_in_current_sound_s += dt.asSeconds();
      if (_elapsed_in_current_sound_s >= _current_sound_duration_s)
      {
         playNextSound();
      }
   }

   if (!_sound_thread_id.has_value())
   {
      applyLoudness(1.0f);
      return;
   }

   applyLoudness(Audio::getInstance().getSampleLoudness(_sound_thread_id.value()).value_or(1.0f));
}

void Wind::playNextSound()
{
   stopPlaying();

   auto next_index = size_t{0};
   const auto looped = (_sounds.size() == 1);

   if (!looped)
   {
      // hearing the same gust twice in a row makes the randomization look broken
      do
      {
         next_index = static_cast<size_t>(std::rand()) % _sounds.size();
      } while (_current_sound_index.has_value() && next_index == _current_sound_index.value());
   }

   const auto& sound = _sounds[next_index];
   const auto duration = Audio::getInstance().getSampleDuration(sound);

   _current_sound_index = next_index;
   _elapsed_in_current_sound_s = 0.0f;
   _current_sound_duration_s = duration.has_value() ? duration->asSeconds() : 0.0f;
   _sound_thread_id = Audio::getInstance().playSample({sound, _audio_update_data._volume, looped});
}

void Wind::stopPlaying()
{
   if (!_sound_thread_id.has_value())
   {
      return;
   }

   Audio::getInstance().stopSample(_sound_thread_id.value());
   _sound_thread_id.reset();
}

void Wind::setAudioEnabled(bool audio_enabled)
{
   if (audio_enabled == _audio_enabled)
   {
      return;
   }

   GameMechanism::setAudioEnabled(audio_enabled);

   if (!audio_enabled)
   {
      stopPlaying();
   }

   // starting is left to updateSound so the sample choice and its bookkeeping live in one place
}

void Wind::setVolume(float volume)
{
   GameMechanism::setVolume(volume);

   if (!_sound_thread_id.has_value())
   {
      return;
   }

   Audio::getInstance().setVolume(_sound_thread_id.value(), volume);
}

void Wind::updateLeaves(const sf::Time& dt)
{
   if (_leaves.empty())
   {
      return;
   }

   const auto dt_s = dt.asSeconds();
   const auto perpendicular = sf::Vector2f{-_direction_screen.y, _direction_screen.x};
   const auto velocity_px_s = _leaf_settings._velocity_px_s * _leaf_factor;
   const auto alpha_max = std::clamp(_leaf_settings._alpha, 0.0f, 1.0f) * 255.0f;
   const auto frame_size_px = static_cast<float>(_leaf_settings._frame_size_px);

   for (auto& leaf : _leaves)
   {
      leaf._age_s += dt_s;

      const auto jitter = std::sin(leaf._age_s * leaf._jitter_frequency_hz * two_pi + leaf._jitter_phase);
      const auto forward_px_s = velocity_px_s * leaf._speed_factor;
      const auto sideways_px_s = forward_px_s * _leaf_settings._jitter_amount * jitter;

      leaf._position_px += (_direction_screen * forward_px_s + perpendicular * sideways_px_s) * dt_s;

      if (leaf._age_s > leaf._lifetime_s || !_area.contains(leaf._position_px))
      {
         respawnLeaf(leaf, false);
         continue;
      }

      const auto fade_duration_s = std::min(leaf_fade_duration_s, leaf._lifetime_s * 0.5f);
      auto fade = 1.0f;
      if (leaf._age_s < fade_duration_s)
      {
         fade = leaf._age_s / fade_duration_s;
      }
      else if (leaf._lifetime_s - leaf._age_s < fade_duration_s)
      {
         fade = (leaf._lifetime_s - leaf._age_s) / fade_duration_s;
      }

      const auto frame =
         static_cast<int32_t>((leaf._age_s + leaf._animation_offset_s) * _leaf_settings._animation_speed) % _leaf_frame_count;
      if (frame != leaf._frame)
      {
         leaf._frame = frame;
         sfcompat::setTextureRect(
            *leaf._sprite,
            sf::IntRect({leaf._frame * _leaf_settings._frame_size_px, 0}, {_leaf_settings._frame_size_px, _leaf_settings._frame_size_px})
         );
      }

      sfcompat::setPosition(*leaf._sprite, leaf._position_px);
      sfcompat::setColor(*leaf._sprite, sf::Color{255, 255, 255, static_cast<uint8_t>(alpha_max * fade)});
   }
}

void Wind::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   // checked here as well so a zone without leaves - a pure force barrier - does not even build
   // the render states
   if (!_enabled || _leaves.empty())
   {
      return;
   }

   draw(target, normal, {});
}

void Wind::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/, const sf::RenderStates& states)
{
   if (!_enabled || _leaves.empty())
   {
      return;
   }

   sf::RenderStates draw_states = states;
   draw_states.texture = _leaf_texture.get();
   draw_states.blendMode = sf::BlendAlpha;

   for (const auto& leaf : _leaves)
   {
      target.draw(*leaf._sprite, draw_states);
   }
}

std::optional<sf::FloatRect> Wind::getBoundingBoxPx()
{
   return _area;
}

const sf::Vector2f& Wind::getDirection() const
{
   return _direction;
}

float Wind::getStrength() const
{
   return _strength;
}
