#include "weather.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "player/player.h"

Weather::Weather(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Weather).name());
}

void Weather::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   auto player_rect = Player::getCurrent()->getPixelRectFloat();

   if (_rect.intersects(player_rect))
   {
      _overlay->draw(target, normal);
   }
}

void Weather::update(const sf::Time& dt)
{
   auto player_rect = Player::getCurrent()->getPixelRectFloat();

   if (_rect.intersects(player_rect))
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
