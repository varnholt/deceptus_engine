#include "weather.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "player/player.h"


Weather::Weather(GameNode* parent)
 : GameNode(parent)
{
}


void Weather::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   auto player_rect = Player::getCurrent()->getPlayerPixelRect();

   if (_rect.intersects(player_rect))
   {
      _overlay->draw(target, normal);
   }
}


void Weather::update(const sf::Time& dt)
{
   auto player_rect = Player::getCurrent()->getPlayerPixelRect();

   if (_rect.intersects(player_rect))
   {
      _overlay->update(dt);
   }
}


std::shared_ptr<Weather> Weather::deserialize(TmxObject* tmx_object)
{
   auto weather = std::make_shared<Weather>();

   weather->_rect = sf::IntRect {
      static_cast<int32_t>(tmx_object->_x_px),
      static_cast<int32_t>(tmx_object->_y_px),
      static_cast<int32_t>(tmx_object->_width_px),
      static_cast<int32_t>(tmx_object->_height_px)
   };

   weather->setZ(static_cast<int32_t>(ZDepth::ForegroundMax));

   if (tmx_object->_name.rfind("rain", 0) == 0)
   {
      weather->_overlay = std::make_shared<RainOverlay>();

      if (tmx_object->_properties)
      {
         const auto z_it                 = tmx_object->_properties->_map.find("z");
         const auto collide_it           = tmx_object->_properties->_map.find("collide");
         const auto drop_count_it        = tmx_object->_properties->_map.find("drop_count");
         const auto fall_through_rate_it = tmx_object->_properties->_map.find("fall_through_rate");

         if (z_it != tmx_object->_properties->_map.end())
         {
            weather->setZ(z_it->second->_value_int.value());
         }

         RainOverlay::RainSettings settings;
         if (collide_it != tmx_object->_properties->_map.end())
         {
            settings._collide = collide_it->second->_value_bool.value();
         }

         if (drop_count_it != tmx_object->_properties->_map.end())
         {
            settings._drop_count = drop_count_it->second->_value_int.value();
         }

         if (fall_through_rate_it != tmx_object->_properties->_map.end())
         {
            settings._fall_through_rate = fall_through_rate_it->second->_value_int.value();
         }

         std::dynamic_pointer_cast<RainOverlay>(weather->_overlay)->setSettings(settings);
      }
   }

   if (tmx_object->_name.rfind("thunderstorm", 0) == 0)
   {
      weather->_overlay = std::make_shared<ThunderstormOverlay>();

      if (tmx_object->_properties)
      {
         const auto z_it = tmx_object->_properties->_map.find("z");

         if (z_it != tmx_object->_properties->_map.end())
         {
            weather->setZ(z_it->second->_value_int.value());
         }
      }

      const auto rect = sf::FloatRect{
         tmx_object->_x_px,
         tmx_object->_y_px,
         tmx_object->_width_px,
         tmx_object->_height_px
      };

      std::dynamic_pointer_cast<ThunderstormOverlay>(weather->_overlay)->setRect(rect);
   }

   return weather;
}

