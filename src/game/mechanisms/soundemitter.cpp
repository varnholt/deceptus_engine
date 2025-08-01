#include "soundemitter.h"

#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/audio/audio.h"

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
   if (_thread_id.has_value())
   {
      Audio::getInstance().stopSample(_thread_id.value());
   }
}

void SoundEmitter::setAudioEnabled(bool audio_enabled)
{
   if (audio_enabled == _audio_enabled)
   {
      return;
   }

   GameMechanism::setAudioEnabled(audio_enabled);

   if (audio_enabled)
   {
      // start playing
      _thread_id = Audio::getInstance().playSample({_filename, _reference_volume, _looped});
   }
   else
   {
      // stop playing
      stopPlaying();
   }
}

void SoundEmitter::setReferenceVolume(float volume)
{
   GameMechanism::setReferenceVolume(volume);

   if (!_thread_id.has_value())
   {
      return;
   }

   Audio::getInstance().setVolume(_thread_id.value(), volume);
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

      Audio::getInstance().addSample(instance->_filename);
   }

   return instance;
}

std::optional<sf::FloatRect> SoundEmitter::getBoundingBoxPx()
{
   return _rect;
}
