#include "objectupdater.h"

#include "framework/math/sfmlmath.h"

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

   const auto rect = mechanism->getBoundingBoxPx().value();
   const auto mechanism_left_px = rect.left;
   const auto mechanism_top_px = rect.top;
   const auto mechanism_height_px = rect.height;
   const auto mechanism_width_px = rect.width;
   const auto pos = sf::Vector2f{mechanism_left_px + mechanism_width_px / 2, mechanism_top_px + mechanism_height_px / 2};

   const auto distance = SfmlMath::length(_player_position - pos);
   const auto range = mechanism->getAudioRange().value();
   if (distance < range._radius_far_px)
   {
      // calculate volume
      std::cout << "update volume" << std::endl;
   }
   else
   {
      // disable sounds for the object
      std::cout << "disable sounds" << std::endl;
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
