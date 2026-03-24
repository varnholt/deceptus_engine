#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <functional>
#include "SFML/Graphics.hpp"

/// \brief triggers actions when the player enters or leaves a configured rectangle.
class SensorRect : public GameMechanism, public GameNode
{
public:
   /// \brief creates a sensor rectangle mechanism.
   /// \param parent owning game node in the scene graph.
   SensorRect(GameNode* parent = nullptr);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "SensorRect".
   std::string_view objectName() const override;

   /// \brief action applied to referenced mechanisms when the sensor fires.
   enum class Action
   {
      Enable,
      Disable,
      Toggle,
   };

   /// \brief event that causes sensor evaluation to fire.
   enum class Event
   {
      OnEnter,
      OnLeave,
   };

   /// \brief checks player overlap transitions and triggers configured actions.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the sensor rectangle in pixel space.
   /// \return area used for player intersection checks.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief initializes rectangle bounds and behavior from tmx properties.
   /// \param data deserialization data for this sensor object.
   void setup(const GameDeserializeData& data);

   /// \brief resolves referenced mechanisms by object id.
   /// \param mechanisms mechanism list searched for the configured reference id.
   void findReference(const std::vector<std::shared_ptr<GameMechanism>>& mechanisms);

   using SensorCallback = std::function<void(const std::string& id)>;

   /// \brief registers a callback executed after sensor actions are processed.
   /// \param callback callback receiving this sensor object id.
   void addSensorCallback(const SensorCallback& callback);

   /// \brief reports whether the player intersected the sensor during the last update.
   /// \return true while the player is inside the sensor rectangle.
   bool playerIntersects() const;

private:
   /// \brief applies the configured action to all resolved references and notifies observers.
   void processAction();

   sf::FloatRect _rect;
   bool _player_intersects = false;
   Event _event = Event::OnEnter;
   Action _action = Action::Enable;
   std::optional<std::string> _reference_id;
   std::vector<std::shared_ptr<GameMechanism>> _references;
   std::vector<SensorCallback> _callbacks;
   bool _observed = false;
};
