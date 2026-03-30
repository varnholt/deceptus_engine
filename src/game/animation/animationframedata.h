#pragma once

#include <memory>
#include <vector>

#include <SFML/Graphics.hpp>

/// \brief sprite-sheet frame layout and timing data used to initialize an animation.
struct AnimationFrameData
{
   /// \brief constructs an empty frame-data container.
   AnimationFrameData() = default;

   /// \brief builds frame rectangles from a sprite sheet and stores matching frame timings.
   /// \param texture sprite-sheet texture containing all frames.
   /// \param origin local origin applied to spawned animations.
   /// \param frame_width width of one frame in pixels.
   /// \param frame_height height of one frame in pixels.
   /// \param frame_count number of consecutive frames to extract.
   /// \param frames_per_row number of frames stored per sprite-sheet row.
   /// \param frame_times playback duration for each extracted frame.
   /// \param start_frame first frame index inside the sprite sheet.
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
