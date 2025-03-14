#pragma once

#include <memory>
#include <vector>

#include <SFML/Graphics.hpp>

struct AnimationFrameData
{
   AnimationFrameData() = default;
   AnimationFrameData(
      const std::shared_ptr<sf::Texture>& texture,
      const sf::Vector2f& origin,
      int32_t frame_width,
      int32_t frame_height,
      int32_t frame_count,
      int32_t frames_per_row,
      const std::vector<sf::Time>& frame_times,
      int32_t start_frame = 0
   );

   std::shared_ptr<sf::Texture> _texture;
   sf::Vector2f _origin;
   std::vector<sf::IntRect> _frames;
   std::vector<sf::Time> _frame_times;
};
