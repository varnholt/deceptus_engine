#include "soundemitter.h"

#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/audio/audio.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

#include <algorithm>
#include <array>

SoundEmitter::SoundEmitter(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(SoundEmitter).name());
   setZ(1);  // bogus z
   _has_audio = true;
}

SoundEmitter::~SoundEmitter()
{
   stopPlaying();
}

std::string_view SoundEmitter::objectName() const
{
   return "SoundEmitter";
}

void SoundEmitter::stopPlaying()
{
   if (!_thread_id.has_value())
   {
      return;
   }

   Audio::getInstance().stopSample(_thread_id.value());
   _thread_id.reset();
}

float SoundEmitter::computeVolume() const
{
   return _audio_update_data._volume * _fade_factor;
}

void SoundEmitter::applyVolume()
{
   if (!_thread_id.has_value())
   {
      return;
   }

   Audio::getInstance().setVolume(_thread_id.value(), computeVolume());
}

// the sample keeps its playback slot for as long as it is still audible, so a fade out has something
// to fade, and gives the slot up once it has reached silence:
//
//   enabled   ______                    ______________
//                   |__________________|
//
//   fade      ______
//                   \_____             ______________
//                         \___________/
//
//   sample    [ playing ........ ][ none ][ playing ....
//                                 ^       ^
//                                 stopped restarted from silence
//
void SoundEmitter::update(const sf::Time& dt)
{
   const auto fade_target = _enabled ? 1.0f : 0.0f;

   // a sample that never started has nothing to fade out, it just stays silent
   if (!_thread_id.has_value() && fade_target < _fade_factor)
   {
      _fade_factor = fade_target;
   }

   if (_fade_duration_s > 0.0f)
   {
      const auto max_step = dt.asSeconds() / _fade_duration_s;
      _fade_factor = std::clamp(_fade_factor + std::clamp(fade_target - _fade_factor, -max_step, max_step), 0.0f, 1.0f);
   }
   else
   {
      _fade_factor = fade_target;
   }

   if (_fade_factor <= 0.0f)
   {
      stopPlaying();
      return;
   }

   if (!_thread_id.has_value())
   {
      // the volume updater decides whether the player is close enough to hear this emitter at all
      if (_audio_enabled)
      {
         _thread_id = Audio::getInstance().playSample({_filename, computeVolume(), _looped});
      }

      return;
   }

   applyVolume();
}

void SoundEmitter::setAudioEnabled(bool audio_enabled)
{
   if (audio_enabled == _audio_enabled)
   {
      return;
   }

   GameMechanism::setAudioEnabled(audio_enabled);

   if (!audio_enabled)
   {
      // stop playing
      stopPlaying();
   }

   // starting is left to update so that the fade and the playback bookkeeping live in one place
}

void SoundEmitter::setVolume(float volume)
{
   GameMechanism::setVolume(volume);

   applyVolume();
}

void SoundEmitter::setReferenceVolume(float volume)
{
   GameMechanism::setReferenceVolume(volume);

   applyVolume();
}

std::shared_ptr<SoundEmitter> SoundEmitter::deserialize(GameNode* parent, const GameDeserializeData& data)
{
   auto instance = std::make_shared<SoundEmitter>(parent);

   instance->_reference_volume = 1.0f;
   instance->_position.x = data._tmx_object->_x_px;
   instance->_position.y = data._tmx_object->_y_px;
   instance->_size.x = data._tmx_object->_width_px;
   instance->_size.y = data._tmx_object->_height_px;
   instance->setObjectId(data._tmx_object->_name);
   instance->_rect =
      sf::FloatRect{{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   // deserialize range data
   if (data._tmx_object->_properties)
   {
      // read audio range properties
      AudioRange audio_range;
      const auto radius_far_px = data._tmx_object->_properties->_map.find("radius_far_px");
      if (radius_far_px != data._tmx_object->_properties->_map.cend())
      {
         audio_range._radius_far_px = radius_far_px->second->_value_float.value();
      }

      const auto volume_far = data._tmx_object->_properties->_map.find("volume_far");
      if (volume_far != data._tmx_object->_properties->_map.cend())
      {
         audio_range._volume_far = volume_far->second->_value_float.value();
      }

      const auto radius_near_px = data._tmx_object->_properties->_map.find("radius_near_px");
      if (radius_near_px != data._tmx_object->_properties->_map.cend())
      {
         audio_range._radius_near_px = radius_near_px->second->_value_float.value();
      }

      const auto volume_near = data._tmx_object->_properties->_map.find("volume_near");
      if (volume_near != data._tmx_object->_properties->_map.cend())
      {
         audio_range._volume_near = volume_near->second->_value_float.value();
      }

      instance->_audio_update_data._range = audio_range;

      // read sample properties
      const auto looped = data._tmx_object->_properties->_map.find("looped");
      if (looped != data._tmx_object->_properties->_map.cend())
      {
         instance->_looped = looped->second->_value_bool.value();
      }

      const auto filename = data._tmx_object->_properties->_map.find("filename");
      if (filename != data._tmx_object->_properties->_map.cend())
      {
         instance->_filename = filename->second->_value_string.value();
      }

      const auto fade_duration_s = data._tmx_object->_properties->_map.find("fade_duration_s");
      if (fade_duration_s != data._tmx_object->_properties->_map.cend())
      {
         instance->_fade_duration_s = fade_duration_s->second->_value_float.value();
      }

      Audio::getInstance().addSample(instance->_filename);
   }

   return instance;
}

std::optional<sf::FloatRect> SoundEmitter::getBoundingBoxPx()
{
   return _rect;
}

namespace
{
static constexpr std::array sound_emitter_properties{
   PropertyInfo{.name = "filename", .type = "string", .default_value = std::string_view{""}, .required = true},
   PropertyInfo{.name = "looped", .type = "bool", .default_value = true},
   PropertyInfo{.name = "fade_duration_s", .type = "float", .default_value = 0.0f},
   PropertyInfo{.name = "radius_near_px", .type = "float", .default_value = 200.0f},
   PropertyInfo{.name = "volume_near", .type = "float", .default_value = 1.0f},
   PropertyInfo{.name = "radius_far_px", .type = "float", .default_value = 600.0f},
   PropertyInfo{.name = "volume_far", .type = "float", .default_value = 0.0f},
};
static constexpr MechanismSchema sound_emitter_schema{
   .type_name = "SoundEmitter",
   .layer_name = "sound_emitters",
   .default_width = 192,
   .default_height = 192,
   .properties = sound_emitter_properties,
};
const auto registered_sound_emitter = []
{
   GameMechanismDeserializerRegistry::instance().registerSchema(sound_emitter_schema);
   return true;
}();
}  // namespace
