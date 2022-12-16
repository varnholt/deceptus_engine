#include "objectupdater.h"

#include "framework/math/sfmlmath.h"

ObjectUpdater::ObjectUpdater()
{
   _thread = std::make_unique<std::thread>(&ObjectUpdater::run, this);
}

void ObjectUpdater::run()
{
   std::lock_guard<std::mutex> guard(_mutex);
   while (!_stopped)
   {
      for (const auto& mechanism_vector : _mechanisms)
      {
         for (const auto& mechanism : *mechanism_vector)
         {
            if (!mechanism->hasAudio())
            {
               continue;
            }

            if (!mechanism->getAudioRange().has_value())
            {
               // just assume to always play
               continue;
            }

            if (!mechanism->getBoundingBoxPx().has_value())
            {
               continue;
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
            }
            else
            {
               // disable sounds for the object
            }
         }
      }

      // std::lock_guard<std::mutex> guard(_mutex);
      // virtual std::optional<sf::FloatRect> getBoundingBoxPx() = 0;
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
