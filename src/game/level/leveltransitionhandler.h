#pragma once

#include <optional>
#include <string>

#include "SFML/Graphics.hpp"

/// \brief switches from the active level to another level when a LevelTransition mechanism is entered.
/// \details a transition is requested by the mechanism and performed later from the game loop: the screen fades out,
///          the state of the level that is left behind is written to the save state, the target level is loaded and
///          the screen fades back in. deferring the switch keeps the level alive while its mechanisms are updating.
class LevelTransitionHandler
{
public:
   /// \brief returns the process-wide level transition handler.
   /// \return reference to the shared handler instance.
   static LevelTransitionHandler& getInstance();

   /// \brief requests a switch to another level.
   /// \details an already pending request is kept, so the first mechanism to fire wins.
   /// \param level_description_filename path of the target level's description json, as listed in levels.json.
   /// \param spawn_position_px where to place the player, or nullopt to use the target level's own start position.
   void request(const std::string& level_description_filename, const std::optional<sf::Vector2f>& spawn_position_px);

   /// \brief performs a pending level transition; expected to be called once per frame.
   void update();

   /// \brief returns the spawn position requested for the level that is currently being loaded, and clears it.
   /// \return spawn position, or nullopt when the level's own start position or checkpoint should be used.
   std::optional<sf::Vector2f> takeSpawnPosition();

   /// \brief discards a pending transition and spawn position.
   void reset();

private:
   LevelTransitionHandler() = default;

   struct Request
   {
      std::string _level_description_filename;
      std::optional<sf::Vector2f> _spawn_position_px;
   };

   std::optional<Request> _pending_request;
   std::optional<sf::Vector2f> _spawn_position_px;
};
