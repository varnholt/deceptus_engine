#include "volumeupdater.h"

#include "framework/math/sfmlmath.h"
#include "game/audio.h"
#include "game/gamestate.h"
#include "game/luainterface.h"

#include <iostream>

float VolumeUpdater::computeDistanceToPlayerPx(const std::shared_ptr<GameMechanism>& mechanism)
{
   const auto rect = mechanism->getBoundingBoxPx().value();
   const auto mechanism_left_px = rect.left;
   const auto mechanism_top_px = rect.top;
   const auto mechanism_height_px = rect.height;
   const auto mechanism_width_px = rect.width;
   const auto pos = sf::Vector2f{mechanism_left_px + mechanism_width_px / 2, mechanism_top_px + mechanism_height_px / 2};
   const auto distance_px = SfmlMath::length(_player_position - pos);

   return distance_px;
}

void VolumeUpdater::setRoomId(const std::optional<int32_t>& room_id)
{
   _room_id = room_id;
}

void VolumeUpdater::updateVolume(const std::shared_ptr<GameMechanism>& mechanism)
{
   if (!mechanism->hasAudio())
   {
      return;
   }

   const auto paused_factor = (GameState::getInstance().getMode() == ExecutionMode::Paused) ? 0.5f : 1.0f;

   switch (mechanism->getAudioUpdateBehavior())
   {
      case AudioUpdateBehavior::RoomBased:
      {
         const auto same_room =
            _room_id.has_value() && mechanism->getRoomId().has_value() && mechanism->getRoomId().value() == _room_id.value();
         mechanism->setAudioEnabled(same_room);
         break;
      }
      case AudioUpdateBehavior::RangeBased:
      {
         if (!mechanism->getAudioRange().has_value())
         {
            return;
         }

         if (!mechanism->getBoundingBoxPx().has_value())
         {
            return;
         }

         const auto range = mechanism->getAudioRange().value();
         const auto distance = computeDistanceToPlayerPx(mechanism);
         const auto within_range = distance < range._radius_far_px;

         if (within_range)
         {
            // calculate volume
            float volume = 0.0f;
            if (distance < range._radius_near_px)
            {
               volume = range._volume_near;
            }
            else
            {
               //    object
               // +---------+
               // |         |      volume near                                  volume far
               // |    +--- | -------- | -------------------------------------------|-----------------
               // |         |         0.7   |                                      0.1
               // +---------+        100px  |                                     1000px
               //                           |
               //                           x = 300px
               //
               // everything below 100px is 0.7
               // everything between 100px and 1000px is between 0.7 and 0.1
               //
               const auto range_width_px = (range._radius_far_px - range._radius_near_px);
               const auto dist_normalized = 1.0f - ((distance - range._radius_near_px) / range_width_px);
               volume = std::lerp(range._volume_far, range._volume_near, dist_normalized);
            }

            // std::cout << "update volume: " << volume.value() << std::endl;
            mechanism->setAudioEnabled(true);
            mechanism->setVolume(volume * paused_factor);
         }
         else
         {
            // disable sounds for the object
            // std::cout << "disable sounds" << std::endl;
            mechanism->setAudioEnabled(false);
         }

         break;
      }
      case AudioUpdateBehavior::AlwaysOn:
      {
         mechanism->setAudioEnabled(true);
         const auto volume = mechanism->getReferenceVolume();
         mechanism->setVolume(volume * paused_factor);
         break;
      }
   }
}

void VolumeUpdater::update()
{
   // update volume of all mechanisms and enemies
   for (const auto* mechanism_vector : _mechanisms)
   {
      for (const auto& mechanism : *mechanism_vector)
      {
         updateVolume(mechanism);
      }
   }

   // update volume of every enemy
   for (const auto& enemy : LuaInterface::instance().getObjectList())
   {
      updateVolume(enemy);
   }
}

void VolumeUpdater::setPlayerPosition(const sf::Vector2f& position)
{
   _player_position = position;
}

void VolumeUpdater::setMechanisms(const std::vector<std::vector<std::shared_ptr<GameMechanism>>*>& mechanisms)
{
   _mechanisms = mechanisms;
}
