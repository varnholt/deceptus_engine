#include "objectupdater.h"

#include "framework/math/sfmlmath.h"
#include "gamestate.h"

#include <iostream>

ObjectUpdater::ObjectUpdater()
{
   _thread = std::make_unique<std::thread>(&ObjectUpdater::run, this);
}

ObjectUpdater::~ObjectUpdater()
{
   _stopped = true;
   _thread->join();

   std::cout << "object updater destroyed" << std::endl;
}

float ObjectUpdater::computeDistanceToPlayerPx(const std::shared_ptr<GameMechanism>& mechanism)
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

void ObjectUpdater::updateVolume(const std::shared_ptr<GameMechanism>& mechanism)
{
   if (!mechanism->hasAudio())
   {
      return;
   }

   if (!mechanism->getAudioRange().has_value())
   {
      // just assume to always play
      return;
   }

   if (!mechanism->getBoundingBoxPx().has_value())
   {
      return;
   }

   const auto distance = computeDistanceToPlayerPx(mechanism);
   const auto range = mechanism->getAudioRange().value();

   if (distance < range._radius_far_px)
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
      const auto paused_factor = (GameState::getInstance().getMode() == ExecutionMode::Paused) ? 0.5f : 1.0f;
      mechanism->setAudioEnabled(true);
      mechanism->setVolume(volume * paused_factor);
   }
   else
   {
      // disable sounds for the object
      // std::cout << "disable sounds" << std::endl;
      mechanism->setAudioEnabled(false);
   }
}

void ObjectUpdater::run()
{
   using namespace std::chrono_literals;

   // virtual std::optional<sf::FloatRect> getBoundingBoxPx() = 0;
   while (!_stopped)
   {
      // update volume of all mechanisms and enemies
      {
         std::lock_guard<std::mutex> guard(_mutex);
         for (const auto& mechanism_vector : _mechanisms)
         {
            for (const auto& mechanism : *mechanism_vector)
            {
               updateVolume(mechanism);
            }
         }

         for (const auto& enemy : _enemies)
         {
            updateVolume(enemy);
         }
      }

      std::this_thread::sleep_for(100ms);
   }
}

void ObjectUpdater::stop()
{
   _stopped = true;
}

void ObjectUpdater::setPlayerPosition(const sf::Vector2f& position)
{
   std::lock_guard<std::mutex> guard(_mutex);
   _player_position = position;
}

void ObjectUpdater::setEnemies(const std::vector<std::shared_ptr<LuaNode>>& enemies)
{
   std::lock_guard<std::mutex> guard(_mutex);
   _enemies = enemies;
}

void ObjectUpdater::setMechanisms(const std::vector<std::vector<std::shared_ptr<GameMechanism>>*>& mechanisms)
{
   std::lock_guard<std::mutex> guard(_mutex);
   _mechanisms = mechanisms;
}
