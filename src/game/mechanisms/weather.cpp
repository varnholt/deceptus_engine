#include "weather.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/level/roomupdater.h"
#include "game/player/player.h"

Weather::Weather(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Weather).name());
}

void Weather::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   if (_wait_until_start_delay_elapsed)
   {
      return;
   }

   if (_limit_effect_to_room && !RoomUpdater::checkCurrentMatchesIds(getRoomIds()))
   {
      return;
   }

   const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
   const auto intersects = _rect.findIntersection(player_rect).has_value();
   if (intersects)
   {
      _overlay->draw(target, normal);
   }
}

void Weather::updateWaitDelay(const sf::Time& dt, bool intersects)
{
   const auto draw_allowed = intersects && matchesRoom();

   // the first frame ever
   if (!_draw_allowed_in_previous_frame.has_value())
   {
      _draw_allowed_in_previous_frame = draw_allowed;

      // if we're already intersecting from the beginning, skip the delay
      if (draw_allowed)
      {
         _elapsed_since_intersect = _effect_start_delay.value_or(FloatSeconds(0));
      }
   }

   if (_effect_start_delay.has_value())
   {
      // reset elapsed time when intersection state changes
      if (draw_allowed && !_draw_allowed_in_previous_frame.value())
      {
         _elapsed_since_intersect = FloatSeconds(0);
      }
      else
      {
         _elapsed_since_intersect += FloatSeconds(dt.asSeconds());
      }

      _wait_until_start_delay_elapsed = _elapsed_since_intersect < _effect_start_delay.value();
   }

   _draw_allowed_in_previous_frame = draw_allowed;
}

bool Weather::matchesRoom() const
{
   if (!_limit_effect_to_room)
   {
      return true;
   }

   return RoomUpdater::checkCurrentMatchesIds(getRoomIds());
}

void Weather::update(const sf::Time& dt)
{
   const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
   const auto intersects = _rect.findIntersection(player_rect).has_value();
   updateWaitDelay(dt, intersects);

   if (intersects && matchesRoom() && !_wait_until_start_delay_elapsed)
   {
      _overlay->update(dt);
   }
}

std::optional<sf::FloatRect> Weather::getBoundingBoxPx()
{
   return _rect;
}

std::shared_ptr<Weather> Weather::deserialize(GameNode* parent, const GameDeserializeData& data)
{
   auto weather = std::make_shared<Weather>(parent);
   weather->setObjectId(data._tmx_object->_name);

   weather->_rect =
      sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

   weather->setZ(static_cast<int32_t>(ZDepth::ForegroundMax));

   // generic settings
   if (data._tmx_object->_properties)
   {
      const auto limit_effect_to_room_it = data._tmx_object->_properties->_map.find("limit_effect_to_room");
      const auto effect_start_delay_s_it = data._tmx_object->_properties->_map.find("effect_start_delay_s");

      if (limit_effect_to_room_it != data._tmx_object->_properties->_map.end())
      {
         weather->_limit_effect_to_room = limit_effect_to_room_it->second->_value_bool.value();
      }

      if (effect_start_delay_s_it != data._tmx_object->_properties->_map.end())
      {
         weather->_effect_start_delay = FloatSeconds(effect_start_delay_s_it->second->_value_float.value());
      }
   }

   // rain settings
   if (data._tmx_object->_name.rfind("rain", 0) == 0)
   {
      weather->_overlay = std::make_shared<RainOverlay>();

      if (data._tmx_object->_properties)
      {
         const auto z_it = data._tmx_object->_properties->_map.find("z");
         const auto collide_it = data._tmx_object->_properties->_map.find("collide");
         const auto drop_count_it = data._tmx_object->_properties->_map.find("drop_count");
         const auto fall_through_rate_it = data._tmx_object->_properties->_map.find("fall_through_rate");

         if (z_it != data._tmx_object->_properties->_map.end())
         {
            weather->setZ(z_it->second->_value_int.value());
         }

         RainOverlay::RainSettings settings;
         if (collide_it != data._tmx_object->_properties->_map.end())
         {
            settings._collide = collide_it->second->_value_bool.value();
         }

         if (drop_count_it != data._tmx_object->_properties->_map.end())
         {
            settings._drop_count = drop_count_it->second->_value_int.value();
         }

         if (fall_through_rate_it != data._tmx_object->_properties->_map.end())
         {
            settings._fall_through_rate = fall_through_rate_it->second->_value_int.value();
         }

         std::dynamic_pointer_cast<RainOverlay>(weather->_overlay)->setSettings(settings);
      }
   }

   // thunderstorm settings
   if (data._tmx_object->_name.rfind("thunderstorm", 0) == 0)
   {
      weather->_overlay = std::make_shared<ThunderstormOverlay>();

      ThunderstormOverlay::ThunderstormSettings settings;

      if (data._tmx_object->_properties)
      {
         const auto z_it = data._tmx_object->_properties->_map.find("z");
         const auto thunderstorm_time_min_it = data._tmx_object->_properties->_map.find("thunderstorm_time_s");
         const auto silence_time_it = data._tmx_object->_properties->_map.find("silence_time_s");

         if (z_it != data._tmx_object->_properties->_map.end())
         {
            weather->setZ(z_it->second->_value_int.value());
         }

         if (thunderstorm_time_min_it != data._tmx_object->_properties->_map.end())
         {
            settings._thunderstorm_time_s = thunderstorm_time_min_it->second->_value_float.value();
         }

         if (silence_time_it != data._tmx_object->_properties->_map.end())
         {
            settings._silence_time_s = silence_time_it->second->_value_float.value();
         }
      }

      const auto rect =
         sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

      auto thunderstorm = std::dynamic_pointer_cast<ThunderstormOverlay>(weather->_overlay);
      thunderstorm->setRect(rect);
      thunderstorm->setSettings(settings);
   }

   return weather;
}
