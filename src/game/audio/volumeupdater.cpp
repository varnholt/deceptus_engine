#include "volumeupdater.h"

#include "framework/math/sfmlmath.h"
#include "game/level/luainterface.h"
#include "game/state/gamestate.h"
#include "game/weapons/projectile.h"

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
         auto same_room = false;
         if (_room_id.has_value())
         {
            const auto& room_ids = mechanism->getRoomIds();
            same_room = (std::find(room_ids.begin(), room_ids.end(), _room_id.value()) != room_ids.end());
         }

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
            auto volume = 0.0f;
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
   for (auto* mechanism_vector : _mechanisms)
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

void VolumeUpdater::updateProjectiles(const std::set<Projectile*>& projectiles)
{
   for (auto* projectile : projectiles)
   {
      // don't touch audio if there's no audio update data available.
      // that is the case for bullets coming from the player.
      const auto audio_update_data = projectile->getParentAudioUpdateData();
      if (!audio_update_data.has_value())
      {
         continue;
      }

      switch (audio_update_data->_update_behavior)
      {
         case AudioUpdateBehavior::AlwaysOn:
         {
            projectile->setAudioEnabled(true);
            break;
         }
         case AudioUpdateBehavior::RoomBased:
         {
            if (!_room_id.has_value())
            {
               return;
            }

            if (!audio_update_data->_room_ids.empty())
            {
               return;
            }

            auto same_room = false;
            if (_room_id.has_value())
            {
               const auto& room_ids = audio_update_data->_room_ids;
               same_room = (std::find(room_ids.begin(), room_ids.end(), _room_id.value()) != room_ids.end());
            }

            projectile->setAudioEnabled(same_room);
            break;
         }
         case AudioUpdateBehavior::RangeBased:
         {
            if (!audio_update_data->_range.has_value())
            {
               return;
            }

            const auto projectile_position = projectile->getBody()->GetPosition();
            const auto gx = projectile_position.x * PPM;
            const auto gy = projectile_position.y * PPM;
            const auto distance_px = SfmlMath::length(_player_position - sf::Vector2f{gx, gy});
            const auto within_range = distance_px < 600.0f;

            projectile->setAudioEnabled(within_range);

            break;
         }
      }
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
