#include "room.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "cameraroomlock.h"
#include "camerasystem.h"
#include "gameconfiguration.h"
#include "screentransition.h"
#include "fadetransitioneffect.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tools/log.h"
#include "framework/tools/timer.h"
#include "player/player.h"

/*

   room a_0
   +--------------------------------------------------------------------------+
   |                                                                          |
   |  screen                                                                  |
   |  +---------------/.\----------------+                                    |
   |  |                |                 |                                    |
   |  |                |                 |                                    |
   |  |                |                 |                                    |
   |  <----------------+----------------->                                    |
   |  |                |                 |                                    |
   |  |                |                 |                                    |
   |  |                |                 |                                    |
   |  +---------------\./----------------+                                    |
   |                                                                          |
   |                                    room a_1                              |
   +------------------------------------+-------------------------------------+
                                        |                                     |
                                        |                                     |
                                        |                                     |
                                        |                                     |
                                        |                                     |
                                        |                                     |
                                        |                                     |
                                        |                                     |
                                        |                                     |
                                        +-------------------------------------+

*/


Room::Room(const sf::FloatRect& rect)
{
   static int32_t __id = 0;

   _rects.push_back(rect);
   _id = __id;
   __id++;
}


std::vector<sf::FloatRect>::const_iterator Room::findRect(const sf::Vector2f& p) const
{
   const auto it = std::find_if(
         _rects.begin(),
         _rects.end(),
         [p](const sf::FloatRect& rect){
            return rect.contains(p);
         }
      );

   return it;
}


std::shared_ptr<Room> Room::find(const sf::Vector2f& p, const std::vector<std::shared_ptr<Room>>& rooms)
{
   const auto room_it = std::find_if(rooms.begin(), rooms.end(), [p, rooms](const std::shared_ptr<Room>& r){
         const auto& it = r->findRect(p);
         return (it != r->_rects.end());
      }
   );

   return (room_it != rooms.end()) ? (*room_it) : nullptr;
}


void Room::deserialize(TmxObject* tmx_object, std::vector<std::shared_ptr<Room>>& rooms)
{
   // ignore invalid rects
   const auto config = GameConfiguration::getInstance();
   if (tmx_object->_width_px < config._view_width)
   {
      Log::Error() << "ignoring rect, room width smaller than screen width";
      return;
   }

   if (tmx_object->_height_px < config._view_height)
   {
      Log::Error() << "ignoring rect, room height smaller than screen height";
      return;
   }

   // read key from tmx object
   std::istringstream f(tmx_object->_name);
   std::string key;
   if (!getline(f, key, '_'))
   {
      key = tmx_object->_name;
   }

   if (key.empty())
   {
      Log::Error() << "ignoring unnamed room";
      return;
   }

   auto rect = sf::FloatRect{
      tmx_object->_x_px,
      tmx_object->_y_px,
      tmx_object->_width_px,
      tmx_object->_height_px
   };

   // check if room already exists
   const auto it = std::find_if(rooms.begin(), rooms.end(), [key](const std::shared_ptr<Room>& r){
         return (r->_name == key);
      }
   );

   if (it == rooms.end())
   {
      // create new room
      auto room = std::make_shared<Room>(rect);
      room->_name = key;

      // deserialize room properties
      if (tmx_object->_properties)
      {
         const auto transition_it = tmx_object->_properties->_map.find("transition");
         if (transition_it != tmx_object->_properties->_map.end())
         {
            if (transition_it->second->_value_string == "fade_out_fade_in")
            {
               room->_transition_effect = TransitionEffect::FadeOutFadeIn;
            }
         }

         const auto fade_in_speed_it = tmx_object->_properties->_map.find("fade_in_speed");
         if (fade_in_speed_it != tmx_object->_properties->_map.end())
         {
            room->_fade_in_speed = fade_in_speed_it->second->_value_float.value();
         }

         const auto fade_out_speed_it = tmx_object->_properties->_map.find("fade_out_speed");
         if (fade_out_speed_it != tmx_object->_properties->_map.end())
         {
            room->_fade_out_speed = fade_out_speed_it->second->_value_float.value();
         }

         const auto delay_between_effects_it = tmx_object->_properties->_map.find("delay_between_effects_ms");
         if (delay_between_effects_it != tmx_object->_properties->_map.end())
         {
            room->_delay_between_effects_ms = std::chrono::milliseconds{*delay_between_effects_it->second->_value_int};
         }

         const auto camera_sync_after_fade_out_it = tmx_object->_properties->_map.find("camera_sync_after_fade_out");
         if (camera_sync_after_fade_out_it != tmx_object->_properties->_map.end())
         {
            room->_camera_sync_after_fade_out = camera_sync_after_fade_out_it->second->_value_bool.value();
         }

         const auto delay_it = tmx_object->_properties->_map.find("camera_lock_delay_ms");
         if (delay_it != tmx_object->_properties->_map.end())
         {
            room->_camera_lock_delay = std::chrono::milliseconds{*delay_it->second->_value_int};
         }

         // read start positions if available
         const auto start_position_l_x_it = tmx_object->_properties->_map.find("start_position_left_x_px");
         const auto start_position_l_y_it = tmx_object->_properties->_map.find("start_position_left_y_px");
         if (start_position_l_x_it != tmx_object->_properties->_map.end())
         {
            room->_start_position_l = {
               start_position_l_x_it->second->_value_int.value(),
               start_position_l_y_it->second->_value_int.value()
            };
         }

         const auto start_position_r_x_it = tmx_object->_properties->_map.find("start_position_right_x_px");
         const auto start_position_r_y_it = tmx_object->_properties->_map.find("start_position_right_y_px");
         if (start_position_r_x_it != tmx_object->_properties->_map.end())
         {
            room->_start_position_r = {
               start_position_r_x_it->second->_value_int.value(),
               start_position_r_y_it->second->_value_int.value()
            };
         }

         const auto start_position_t_x_it = tmx_object->_properties->_map.find("start_position_top_x_px");
         const auto start_position_t_y_it = tmx_object->_properties->_map.find("start_position_top_y_px");
         if (start_position_t_x_it != tmx_object->_properties->_map.end())
         {
            room->_start_position_t = {
               start_position_t_x_it->second->_value_int.value(),
               start_position_t_y_it->second->_value_int.value()
            };
         }

         const auto start_position_b_x_it = tmx_object->_properties->_map.find("start_position_bottom_x_px");
         const auto start_position_b_y_it = tmx_object->_properties->_map.find("start_position_bottom_y_px");
         if (start_position_b_x_it != tmx_object->_properties->_map.end())
         {
            room->_start_position_b = {
               start_position_b_x_it->second->_value_int.value(),
               start_position_b_y_it->second->_value_int.value()
            };
         }

         const auto start_offset_l_x_it = tmx_object->_properties->_map.find("start_offset_left_x_px");
         const auto start_offset_l_y_it = tmx_object->_properties->_map.find("start_offset_left_y_px");
         if (start_offset_l_x_it != tmx_object->_properties->_map.end())
         {
            auto offset_y = 0;
            if (start_offset_l_y_it != tmx_object->_properties->_map.end())
            {
               offset_y = start_offset_l_y_it->second->_value_int.value();
            }

            room->_start_offset_l = {start_offset_l_x_it->second->_value_int.value(), offset_y};
         }

         const auto start_offset_r_x_it = tmx_object->_properties->_map.find("start_offset_right_x_px");
         const auto start_offset_r_y_it = tmx_object->_properties->_map.find("start_offset_right_y_px");
         if (start_offset_r_x_it != tmx_object->_properties->_map.end())
         {
            auto offset_y = 0;
            if (start_offset_r_y_it != tmx_object->_properties->_map.end())
            {
               offset_y = start_offset_r_y_it->second->_value_int.value();
            }

            room->_start_offset_r = {start_offset_r_x_it->second->_value_int.value(), offset_y};
         }
      }

      rooms.push_back(room);
      // Log::Info() << "adding room: " << key;
   }
   else
   {
      // merge room
      auto& room = *it;

      // test for overlaps
      if (std::any_of(room->_rects.begin(), room->_rects.end(), [rect](const sf::FloatRect& r){
               return r.intersects(rect);
            }
         )
      )
      {
         Log::Error() << "bad rect intersection for room " << key;
      }

      room->_rects.push_back(rect);
   }
}


std::unique_ptr<ScreenTransition> Room::makeFadeTransition()
{
   auto screen_transition = std::make_unique<ScreenTransition>();
   auto fade_out = std::make_shared<FadeTransitionEffect>();
   auto fade_in = std::make_shared<FadeTransitionEffect>();

   fade_out->_direction = FadeTransitionEffect::Direction::FadeOut;
   fade_out->_speed = _fade_out_speed;

   fade_in->_direction = FadeTransitionEffect::Direction::FadeIn;
   fade_in->_value = 1.0f;
   fade_in->_speed = _fade_in_speed;

   screen_transition->_effect_1 = fade_out;
   screen_transition->_effect_2 = fade_in;

   screen_transition->_delay_between_effects_ms = _delay_between_effects_ms;

   return std::move(screen_transition);
}


void Room::movePlayerToRoomStartPosition()
{
   const auto entered_direction = enteredDirection(Player::getCurrent()->getPixelPositionf());

   if (_start_position_l.has_value() && (entered_direction == EnteredDirection::Left))
   {
      Player::getCurrent()->setBodyViaPixelPosition(
         static_cast<float>(_start_position_l.value().x),
         static_cast<float>(_start_position_l.value().y)
      );
   }
   else if (_start_position_r.has_value() && (entered_direction == EnteredDirection::Right))
   {
      Player::getCurrent()->setBodyViaPixelPosition(
         static_cast<float>(_start_position_r.value().x),
         static_cast<float>(_start_position_r.value().y)
      );
   }
   else if (_start_position_t.has_value() && (entered_direction == EnteredDirection::Top))
   {
      Player::getCurrent()->setBodyViaPixelPosition(
         static_cast<float>(_start_position_t.value().x),
         static_cast<float>(_start_position_t.value().y)
      );
   }
   else if (_start_position_b.has_value() && (entered_direction == EnteredDirection::Bottom))
   {
      Player::getCurrent()->setBodyViaPixelPosition(
         static_cast<float>(_start_position_b.value().x),
         static_cast<float>(_start_position_b.value().y)
      );
   }

   if (_start_offset_l.has_value() && (entered_direction == EnteredDirection::Left))
   {
      auto player_pos = Player::getCurrent()->getPixelPositioni();
      player_pos += _start_offset_l.value();
      Player::getCurrent()->setBodyViaPixelPosition(
         static_cast<float>(player_pos.x),
         static_cast<float>(player_pos.y)
      );
   }
   else if (_start_offset_r.has_value() && (entered_direction == EnteredDirection::Right))
   {
      auto player_pos = Player::getCurrent()->getPixelPositioni();
      player_pos += _start_offset_r.value();
      Player::getCurrent()->setBodyViaPixelPosition(
         static_cast<float>(player_pos.x),
         static_cast<float>(player_pos.y)
      );
   }
}


void Room::syncCamera()
{
   if (_camera_sync_after_fade_out)
   {
      _camera_locked = false;
      CameraRoomLock::setRoom(getptr());
      CameraSystem::getCameraSystem().syncNow();

      // apply room start position if available
      movePlayerToRoomStartPosition();
   }
}


void Room::startTransition()
{
   if (!_transition_effect.has_value())
   {
      return;
   }

   switch (_transition_effect.value())
   {
      case TransitionEffect::FadeOutFadeIn:
      {
         auto screen_transition = makeFadeTransition();
         screen_transition->_callbacks_effect_1_ended.push_back([this](){syncCamera();});
         screen_transition->_callbacks_effect_2_ended.push_back([](){ScreenTransitionHandler::getInstance().pop();});
         screen_transition->startEffect1();
         ScreenTransitionHandler::getInstance().push(std::move(screen_transition));
         break;
      }
   }
}


void Room::lockCamera()
{
   _camera_locked = true;

   // delay shall be retrieved from room properties
   Timer::add(_camera_lock_delay.value(),
      [this](){
         // could be that the camera has been unlocked in the meantime
         if (_camera_locked)
         {
            CameraRoomLock::setRoom(getptr());
            _camera_locked = false;
         }
      },
      Timer::Type::Singleshot
   );
}


Room::EnteredDirection Room::enteredDirection(const sf::Vector2f& player_pos_px) const
{
   static constexpr auto eps_px = 100;

   if (activeRect(player_pos_px).has_value())
   {
      const auto r = activeRect(player_pos_px).value();

      sf::FloatRect rect_l{r.left,                    r.top,                     eps_px,  r.height};
      sf::FloatRect rect_r{r.left + r.width - eps_px, r.top,                     eps_px,  r.height};
      sf::FloatRect rect_t{r.left,                    r.top,                     r.width, eps_px};
      sf::FloatRect rect_b{r.left,                    r.top + r.height - eps_px, r.width, eps_px};

      if (rect_l.contains(player_pos_px))
      {
         return Room::EnteredDirection::Left;
      }

      if (rect_r.contains(player_pos_px))
      {
         return Room::EnteredDirection::Right;
      }

      if (rect_t.contains(player_pos_px))
      {
         return Room::EnteredDirection::Top;
      }

      if (rect_b.contains(player_pos_px))
      {
         return Room::EnteredDirection::Bottom;
      }
   }

   return EnteredDirection::Invalid;
}


std::optional<sf::FloatRect> Room::activeRect(const sf::Vector2f& player_pos_px) const
{
   std::optional<sf::FloatRect> rect;

   const auto& it = std::find_if(_rects.begin(), _rects.end(), [player_pos_px](const auto& rect){
         return rect.contains(player_pos_px);}
      );

   if (it != _rects.end())
   {
      rect = *it;
   }

   return rect;
}
