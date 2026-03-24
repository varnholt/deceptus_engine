#ifndef VOLUMEUPDATER_H
#define VOLUMEUPDATER_H

#include <SFML/Graphics.hpp>

#include "game/level/luanode.h"
#include "game/mechanisms/gamemechanism.h"

#include <set>
#include <thread>

class Projectile;

/// \brief updates mechanism and projectile audio enablement and volume based on room and player distance rules.
class VolumeUpdater
{
public:
   /// \brief constructs a volume updater with empty mechanism sources.
   VolumeUpdater() = default;

   /// \brief updates audio state for registered mechanisms and all lua-managed enemies.
   void update();
   /// \brief updates projectile audio enablement according to parent audio update behavior metadata.
   /// \param projectiles active projectile pointers to evaluate.
   void updateProjectiles(const std::set<Projectile*>& projectiles);

   /// \brief sets the current player position used for distance-based volume calculations.
   /// \param position player world position in pixels.
   void setPlayerPosition(const sf::Vector2f& position);
   /// \brief sets the mechanism container lists whose objects should be processed by update().
   /// \param mechanisms list of mechanism vectors owned by the level systems.
   void setMechanisms(const std::vector<std::vector<std::shared_ptr<GameMechanism>>*>& mechanisms);
   /// \brief sets the player's current room id for room-based audio enable checks.
   /// \param room_id active room id, or std::nullopt when no room is known.
   void setRoomId(const std::optional<int32_t>& room_id);

private:
   /// \brief applies the mechanism's configured audio update behavior to its enabled state and volume.
   /// \param mechanism mechanism whose audio state should be refreshed.
   void updateVolume(const std::shared_ptr<GameMechanism>& mechanism);
   /// \brief computes distance from the player to the center of the mechanism bounding box.
   /// \param mechanism mechanism whose bounding box center is used for distance calculation.
   /// \return euclidean player-to-mechanism distance in pixels.
   float computeDistanceToPlayerPx(const std::shared_ptr<GameMechanism>& mechanism);

   std::vector<std::vector<std::shared_ptr<GameMechanism>>*> _mechanisms;
   sf::Vector2f _player_position;

   std::unique_ptr<std::thread> _thread;
   std::atomic<bool> _stopped{false};
   std::optional<int32_t> _room_id;
};

#endif  // VOLUMEUPDATER_H
