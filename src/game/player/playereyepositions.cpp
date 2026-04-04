#include "playereyepositions.h"

#include "game/animation/animation.h"

#include <fstream>
#include "json/json.hpp"

PlayerEyePositions::PlayerEyePositions()
{
   load();
}

std::optional<sf::Vector2f> PlayerEyePositions::getEyePosition(const std::string& animation_id, int32_t frame_index) const
{
   const auto eye_position_it = _eye_positions.find(animation_id);
   if (eye_position_it == _eye_positions.end() || eye_position_it->second.empty())
   {
      return std::nullopt;
   }

   const auto& positions = eye_position_it->second;
   const auto clamped_index = std::min(static_cast<size_t>(frame_index), positions.size() - 1);
   return positions[clamped_index];
}

std::optional<sf::Vector2f> PlayerEyePositions::getEyePosition(const std::shared_ptr<Animation>& animation) const
{
   return getEyePosition(animation->_name, animation->_current_frame);
}

void PlayerEyePositions::load()
{
   std::ifstream file("data/sprites/eye_positions.json");
   if (!file.is_open())
   {
      return;
   }

   nlohmann::json json_data;
   file >> json_data;

   for (const auto& [key, value] : json_data.items())
   {
      const auto x_positions_it = value.find("eye_positions_x");
      const auto y_positions_it = value.find("eye_positions_y");
      if (x_positions_it == value.end() || y_positions_it == value.end())
      {
         continue;
      }

      const auto& x_positions = *x_positions_it;
      const auto& y_positions = *y_positions_it;

      // ensure both arrays have the same length
      const auto count = std::min(x_positions.size(), y_positions.size());

      std::vector<sf::Vector2f> positions;
      positions.reserve(count);

      for (size_t i = 0; i < count; ++i)
      {
         positions.emplace_back(static_cast<float>(x_positions[i]), static_cast<float>(y_positions[i]));
      }

      _eye_positions[key] = std::move(positions);
   }
}
