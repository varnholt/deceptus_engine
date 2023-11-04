#include "room.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "cameraroomlock.h"
#include "camerasystem.h"
#include "fadetransitioneffect.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"
#include "framework/tools/timer.h"
#include "gameconfiguration.h"
#include "player/player.h"
#include "screentransition.h"

namespace
{
constexpr auto eps_px = 100;
}

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

Room::Room(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Room).name());

   static int32_t __id = 0;

   _id = __id;
   __id++;
}

std::vector<Room::SubRoom>::const_iterator Room::findSubRoom(const sf::Vector2f& p) const
{
   const auto it = std::find_if(_sub_rooms.begin(), _sub_rooms.end(), [p](const auto& sub_room) { return sub_room._rect.contains(p); });
   return it;
}

std::vector<Room::SubRoom>::const_iterator Room::findSubRoom(const sf::FloatRect& rect) const
{
   const auto it =
      std::find_if(_sub_rooms.begin(), _sub_rooms.end(), [rect](const auto& sub_room) { return sub_room._rect.intersects(rect); });
   return it;
}

std::shared_ptr<Room> Room::find(const sf::Vector2f& p, const std::vector<std::shared_ptr<Room>>& rooms)
{
   const auto room_it = std::find_if(
      rooms.begin(),
      rooms.end(),
      [p, rooms](const std::shared_ptr<Room>& r)
      {
         const auto& it = r->findSubRoom(p);
         return (it != r->_sub_rooms.end());
      }
   );

   return (room_it != rooms.end()) ? (*room_it) : nullptr;
}

std::shared_ptr<Room> Room::find(const sf::FloatRect& rect, const std::vector<std::shared_ptr<Room>>& rooms)
{
   const auto room_it = std::find_if(
      rooms.begin(),
      rooms.end(),
      [rect, rooms](const std::shared_ptr<Room>& r)
      {
         const auto& it = r->findSubRoom(rect);
         return (it != r->_sub_rooms.end());
      }
   );

   return (room_it != rooms.end()) ? (*room_it) : nullptr;
}

std::vector<std::shared_ptr<Room>> Room::findAll(const sf::FloatRect& rect, const std::vector<std::shared_ptr<Room>>& rooms)
{
   std::vector<std::shared_ptr<Room>> matching_rooms;

   std::copy_if(
      rooms.begin(),
      rooms.end(),
      std::back_inserter(matching_rooms),
      [rect](const std::shared_ptr<Room>& r)
      {
         const auto& it = r->findSubRoom(rect);
         return (it != r->_sub_rooms.end());
      }
   );

   return matching_rooms;
}

namespace
{
std::vector<Room::RoomEnterArea> _enter_areas;
}  // namespace

void Room::mergeStartAreas(const std::vector<std::shared_ptr<Room>>& rooms)
{
   for (const auto& area : _enter_areas)
   {
      for (auto& room : rooms)
      {
         auto it = std::find_if(
            room->_sub_rooms.begin(), room->_sub_rooms.end(), [area](const auto& sub_room) { return sub_room._rect.intersects(area._area); }
         );

         if (it != room->_sub_rooms.end())
         {
            it->_enter_areas.insert(it->_enter_areas.begin(), area);
         }
      }
   }
   _enter_areas.clear();
}

void Room::deserialize(GameNode* parent, const GameDeserializeData& data, std::vector<std::shared_ptr<Room>>& rooms)
{
   if (data._tmx_object->_name.starts_with("enter_area"))
   {
      std::cout << "skip enter area";
      Room::RoomEnterArea area;
      area._area =
         sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

      _enter_areas.push_back(area);
      return;
   }

   // ignore invalid rects
   const auto& config = GameConfiguration::getInstance();
   if (data._tmx_object->_width_px < config._view_width)
   {
      Log::Error() << "ignoring rect, room width smaller than screen width";
      return;
   }

   if (data._tmx_object->_height_px < config._view_height)
   {
      Log::Error() << "ignoring rect, room height smaller than screen height";
      return;
   }

   // deserialize room group
   std::optional<std::string> group_name;
   if (data._tmx_object->_properties)
   {
      const auto transition_it = data._tmx_object->_properties->_map.find("group");
      if (transition_it != data._tmx_object->_properties->_map.end())
      {
         group_name = *transition_it->second->_value_string;
      }
   }

   // check if room belongs to a group of rooms
   const auto group_it =
      std::find_if(rooms.begin(), rooms.end(), [group_name](const std::shared_ptr<Room>& r) { return (r->_group_name == group_name); });

   // create new room if room has no group associated or room with a group assigned has not been created yet
   std::shared_ptr<Room> room;
   if (!group_name.has_value() || group_it == rooms.end())
   {
      room = std::make_shared<Room>(parent);
      room->_group_name = group_name;
      room->setObjectId(data._tmx_object->_name);
      rooms.push_back(room);

      // deserialize room properties
      if (data._tmx_object->_properties)
      {
         const auto transition_it = data._tmx_object->_properties->_map.find("transition");
         if (transition_it != data._tmx_object->_properties->_map.end())
         {
            if (transition_it->second->_value_string == "fade_out_fade_in")
            {
               room->_transition_effect = TransitionEffect::FadeOutFadeIn;
            }
         }

         const auto fade_in_speed_it = data._tmx_object->_properties->_map.find("fade_in_speed");
         if (fade_in_speed_it != data._tmx_object->_properties->_map.end())
         {
            room->_fade_in_speed = fade_in_speed_it->second->_value_float.value();
         }

         const auto fade_out_speed_it = data._tmx_object->_properties->_map.find("fade_out_speed");
         if (fade_out_speed_it != data._tmx_object->_properties->_map.end())
         {
            room->_fade_out_speed = fade_out_speed_it->second->_value_float.value();
         }

         const auto delay_between_effects_it = data._tmx_object->_properties->_map.find("delay_between_effects_ms");
         if (delay_between_effects_it != data._tmx_object->_properties->_map.end())
         {
            room->_delay_between_effects_ms = std::chrono::milliseconds{*delay_between_effects_it->second->_value_int};
         }

         const auto camera_sync_after_fade_out_it = data._tmx_object->_properties->_map.find("camera_sync_after_fade_out");
         if (camera_sync_after_fade_out_it != data._tmx_object->_properties->_map.end())
         {
            room->_camera_sync_after_fade_out = camera_sync_after_fade_out_it->second->_value_bool.value();
         }

         const auto delay_it = data._tmx_object->_properties->_map.find("camera_lock_delay_ms");
         if (delay_it != data._tmx_object->_properties->_map.end())
         {
            room->_camera_lock_delay = std::chrono::milliseconds{*delay_it->second->_value_int};
         }
      }
   }
   else
   {
      // merge room
      room = *group_it;
   }

   Room::SubRoom sub_room;
   sub_room._rect =
      sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

   // read start positions if available
   sub_room.readEntracePositions(data);

   room->_sub_rooms.push_back(sub_room);

   // test for overlaps
   if (std::any_of(
          room->_sub_rooms.begin(), room->_sub_rooms.end(), [sub_room](const auto& room) { return room._rect.intersects(sub_room._rect); }
       ))
   {
      Log::Error() << "bad rect intersection for room " << data._tmx_object->_name;
   }

   // Log::Info() << "adding room: " << key;
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

   return screen_transition;
}

void Room::movePlayerToRoomStartPosition()
{
   const auto player_pos_px = Player::getCurrent()->getPixelPositionFloat();

   const auto active_sub_room = activeSubRoom(player_pos_px);
   if (!active_sub_room.has_value())
   {
      return;
   }

   const auto entered_direction = (*active_sub_room).findEnteredArea(player_pos_px);
   if (!entered_direction.has_value())
   {
      return;
   }

   const auto area = entered_direction.value();

   if (area._start_position.has_value())
   {
      Player::getCurrent()->setBodyViaPixelPosition(
         static_cast<float>(area._start_position.value().x), static_cast<float>(area._start_position.value().y)
      );
   }

   if (area._start_offset.has_value())
   {
      auto player_pos = Player::getCurrent()->getPixelPositionInt();
      player_pos += area._start_offset.value();
      Player::getCurrent()->setBodyViaPixelPosition(static_cast<float>(player_pos.x), static_cast<float>(player_pos.y));
   }
}

void Room::syncCamera()
{
   _camera_locked = false;
   CameraRoomLock::setRoom(getptr());
   CameraSystem::getInstance().syncNow();
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
         screen_transition->_callbacks_effect_1_ended.push_back(
            [this]()
            {
               if (_camera_sync_after_fade_out)
               {
                  syncCamera();
               }

               // apply room start position if available
               movePlayerToRoomStartPosition();
            }
         );
         screen_transition->_callbacks_effect_2_ended.push_back([]() { ScreenTransitionHandler::getInstance().pop(); });
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
   Timer::add(
      _camera_lock_delay.value(),
      [this]()
      {
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

void Room::SubRoom::readEntracePositions(const GameDeserializeData& data)
{
   sf::FloatRect rect_l{_rect.left, _rect.top, eps_px, _rect.height};
   sf::FloatRect rect_r{_rect.left + _rect.width - eps_px, _rect.top, eps_px, _rect.height};
   sf::FloatRect rect_t{_rect.left, _rect.top, _rect.width, eps_px};
   sf::FloatRect rect_b{_rect.left, _rect.top + _rect.height - eps_px, _rect.width, eps_px};

   auto area_left = RoomEnterArea{"left", rect_l, std::nullopt, std::nullopt};
   auto area_right = RoomEnterArea{"right", rect_r, std::nullopt, std::nullopt};
   auto area_top = RoomEnterArea{"top", rect_t, std::nullopt, std::nullopt};
   auto area_bottom = RoomEnterArea{"bottom", rect_b, std::nullopt, std::nullopt};

   // fixed enter area positions, one for each screen corner
   if (data._tmx_object->_properties)
   {
      const auto start_position_l_x_it = data._tmx_object->_properties->_map.find("start_position_left_x_px");
      const auto start_position_l_y_it = data._tmx_object->_properties->_map.find("start_position_left_y_px");
      if (start_position_l_x_it != data._tmx_object->_properties->_map.end())
      {
         area_left._start_position = {start_position_l_x_it->second->_value_int.value(), start_position_l_y_it->second->_value_int.value()};
      }

      const auto start_position_r_x_it = data._tmx_object->_properties->_map.find("start_position_right_x_px");
      const auto start_position_r_y_it = data._tmx_object->_properties->_map.find("start_position_right_y_px");
      if (start_position_r_x_it != data._tmx_object->_properties->_map.end())
      {
         area_right._start_position = {
            start_position_r_x_it->second->_value_int.value(), start_position_r_y_it->second->_value_int.value()};
      }

      const auto start_position_t_x_it = data._tmx_object->_properties->_map.find("start_position_top_x_px");
      const auto start_position_t_y_it = data._tmx_object->_properties->_map.find("start_position_top_y_px");
      if (start_position_t_x_it != data._tmx_object->_properties->_map.end())
      {
         area_top._start_position = {start_position_t_x_it->second->_value_int.value(), start_position_t_y_it->second->_value_int.value()};
      }

      const auto start_position_b_x_it = data._tmx_object->_properties->_map.find("start_position_bottom_x_px");
      const auto start_position_b_y_it = data._tmx_object->_properties->_map.find("start_position_bottom_y_px");
      if (start_position_b_x_it != data._tmx_object->_properties->_map.end())
      {
         area_bottom._start_position = {
            start_position_b_x_it->second->_value_int.value(), start_position_b_y_it->second->_value_int.value()};
      }

      const auto start_offset_l_x_it = data._tmx_object->_properties->_map.find("start_offset_left_x_px");
      const auto start_offset_l_y_it = data._tmx_object->_properties->_map.find("start_offset_left_y_px");
      if (start_offset_l_x_it != data._tmx_object->_properties->_map.end())
      {
         auto offset_y = 0;
         if (start_offset_l_y_it != data._tmx_object->_properties->_map.end())
         {
            offset_y = start_offset_l_y_it->second->_value_int.value();
         }

         area_left._start_offset = {start_offset_l_x_it->second->_value_int.value(), offset_y};
      }

      const auto start_offset_r_x_it = data._tmx_object->_properties->_map.find("start_offset_right_x_px");
      const auto start_offset_r_y_it = data._tmx_object->_properties->_map.find("start_offset_right_y_px");
      if (start_offset_r_x_it != data._tmx_object->_properties->_map.end())
      {
         auto offset_y = 0;
         if (start_offset_r_y_it != data._tmx_object->_properties->_map.end())
         {
            offset_y = start_offset_r_y_it->second->_value_int.value();
         }

         area_right._start_offset = {start_offset_r_x_it->second->_value_int.value(), offset_y};
      }
   }

   _enter_areas.push_back(area_left);
   _enter_areas.push_back(area_right);
   _enter_areas.push_back(area_top);
   _enter_areas.push_back(area_bottom);
}

std::optional<Room::RoomEnterArea> Room::SubRoom::findEnteredArea(const sf::Vector2f& player_pos_px) const
{
   std::optional<RoomEnterArea> enter_area;

   for (auto& area : _enter_areas)
   {
      if (area._area.contains(player_pos_px))
      {
         return area;
      }
   }

   return std::nullopt;
}

std::optional<Room::SubRoom> Room::activeSubRoom(const sf::Vector2f& player_pos_px) const
{
   std::optional<Room::SubRoom> rect;

   const auto& it = std::find_if(
      _sub_rooms.begin(), _sub_rooms.end(), [player_pos_px](const auto& sub_room) { return sub_room._rect.contains(player_pos_px); }
   );

   if (it != _sub_rooms.end())
   {
      rect = *it;
   }

   return rect;
}

std::optional<Room::RoomEnterArea> Room::enteredArea(const sf::Vector2f& player_pos_px) const
{
   const auto active_sub_room = activeSubRoom(player_pos_px);
   if (!active_sub_room.has_value())
   {
      return std::nullopt;
   }

   return (*active_sub_room).findEnteredArea(player_pos_px);
}
