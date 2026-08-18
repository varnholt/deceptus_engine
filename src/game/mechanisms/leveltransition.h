#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <optional>
#include <string>

#include "SFML/Graphics.hpp"

/// \brief loads another level when the player enters a configured rectangle.
/// \details the state of the level that is left behind is written to the save state before it is unloaded,
///          so its mechanisms are restored when the player comes back.
class LevelTransition : public GameMechanism, public GameNode
{
public:
   /// \brief creates a level transition mechanism.
   /// \param parent owning game node in the scene graph.
   LevelTransition(GameNode* parent = nullptr);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "LevelTransition".
   std::string_view objectName() const override;

   /// \brief checks whether the player entered the rectangle and requests the level switch.
   /// \param dt elapsed frame time, unused by this mechanism.
   void update(const sf::Time& dt) override;

   /// \brief returns the transition rectangle in pixel space.
   /// \return area used for player intersection checks.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief initializes rectangle bounds, target level and spawn position from tmx properties.
   /// \param data deserialization data for this transition object.
   void setup(const GameDeserializeData& data);

private:
   sf::FloatRect _rect_px;
   std::string _level_description_filename;
   std::optional<sf::Vector2f> _spawn_position_px;
   bool _player_intersects{false};
   bool _requested{false};
};
