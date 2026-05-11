#pragma once

#include "game/effects/boomeffect.h"
#include "game/effects/lightsystem.h"
#include "game/level/atmosphere.h"
#include "game/level/gamemechanismregistry.h"

#include <box2d/box2d.h>
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class Room;

class LevelInterface
{
public:
   virtual ~LevelInterface() = default;

   /// \brief gets the box2d physics world.
   /// \return shared pointer to the active physics world.
   virtual const std::shared_ptr<b2World>& getWorld() const = 0;

   /// \brief gets the mechanism registry for this level.
   /// \return const reference to the mechanism registry.
   virtual const GameMechanismRegistry& getMechanismRegistry() const = 0;

   /// \brief gets the boom (screenshake/impulse) effect handler.
   /// \return mutable reference to the boom effect.
   virtual BoomEffect& getBoomEffect() = 0;

   /// \brief gets the light system for this level.
   /// \return shared pointer to the light system.
   virtual const std::shared_ptr<LightSystem>& getLightSystem() const = 0;

   /// \brief gets the atmosphere (water, gas, etc.) state for this level.
   /// \return const reference to the atmosphere.
   virtual const Atmosphere& getAtmosphere() const = 0;

   /// \brief gets the player start position in pixels.
   /// \return pixel-space start position.
   virtual const sf::Vector2f& getStartPosition() const = 0;

   /// \brief gets the level view (camera viewport).
   /// \return shared pointer to the SFML view.
   virtual const std::shared_ptr<sf::View>& getLevelView() const = 0;

   /// \brief saves the current level state (checkpoints).
   virtual void saveState() = 0;

   /// \brief tests whether the physics path between two tile positions is unobstructed.
   /// \param a_tl top-left tile coordinate of the start point.
   /// \param b_tl top-left tile coordinate of the end point.
   /// \return true when no physics body blocks the path.
   virtual bool isPhysicsPathClear(const sf::Vector2i& a_tl, const sf::Vector2i& b_tl) const = 0;

   /// \brief zooms the level view by a delta amount.
   /// \param delta zoom delta.
   virtual void zoomBy(float delta) = 0;

   /// \brief zooms the level view in one step.
   virtual void zoomIn() = 0;

   /// \brief zooms the level view out one step.
   virtual void zoomOut() = 0;

   /// \brief resets the level view zoom to default.
   virtual void zoomReset() = 0;

   /// \brief gets the ambient player light instance.
   /// \return shared pointer to the player light, or nullptr if not created.
   virtual const std::shared_ptr<LightSystem::LightInstance>& getPlayerLight() const = 0;

   /// \brief computes the player spawn position in pixels from the description tile start position.
   virtual void loadStartPosition() = 0;

   /// \brief gets all rooms parsed from the level.
   /// \return immutable reference to room list.
   virtual const std::vector<std::shared_ptr<Room>>& getRooms() const = 0;
};
