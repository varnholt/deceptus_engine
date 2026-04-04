#pragma once

#include <SFML/System/Vector2.hpp>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class Animation;

/// \brief manages eye position data for player animation frames.
/// reads eye_positions.json and provides lookups by animation cycle and frame index.
class PlayerEyePositions
{
public:
   /// \brief loads eye position data from the default json file.
   PlayerEyePositions();

   /// \brief retrieves eye position for a given animation cycle and frame number.
   /// if frame exceeds available positions, returns the last position.
   /// if cycle is not found or empty, returns nullopt.
   /// \param animation_id animation cycle identifier (e.g., "player_idle_l").
   /// \param frame_index zero-based frame index.
   /// \return eye position in pixels, or nullopt if not found.
   std::optional<sf::Vector2f> getEyePosition(const std::string& animation_id, int32_t frame_index) const;

   /// \brief retrieves eye position from an animation's current state.
   /// extracts name and frame from the animation and returns the corresponding eye position.
   /// \param animation shared pointer to the animation instance.
   /// \return eye position in pixels, or nullopt if animation is null or not found.
   std::optional<sf::Vector2f> getEyePosition(const std::shared_ptr<Animation>& animation) const;

private:
   /// \brief loads and parses the json file into the position map.
   void load();

   std::unordered_map<std::string, std::vector<sf::Vector2f>> _eye_positions;
};
