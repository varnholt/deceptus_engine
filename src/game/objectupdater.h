#ifndef OBJECTUPDATER_H
#define OBJECTUPDATER_H

#include <SFML/Graphics.hpp>

#include "gamemechanism.h"
#include "luanode.h"

#include <thread>

class ObjectUpdater
{
public:
   ObjectUpdater() = default;
   virtual ~ObjectUpdater();

   void update();

   void setPlayerPosition(const sf::Vector2f& position);
   void setMechanisms(const std::vector<std::vector<std::shared_ptr<GameMechanism>>*>& mechanisms);

private:
   void updateVolume(const std::shared_ptr<GameMechanism>& mechanism);
   float computeDistanceToPlayerPx(const std::shared_ptr<GameMechanism>& mechanism);

   std::vector<std::vector<std::shared_ptr<GameMechanism>>*> _mechanisms;
   sf::Vector2f _player_position;

   std::unique_ptr<std::thread> _thread;
   std::atomic<bool> _stopped{false};
};

#endif // OBJECTUPDATER_H
