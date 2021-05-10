#include "animationframedata.h"


//   0   1   2   3   4   5   6   7
// +---+---+---+---+---+---+---+---+
// |   |   |   |   |   |###|###|###| 0
// +---+---+---+---+---+---+---+---+
// |###|###|###|   |   |   |   |   | 1
// +---+---+---+---+---+---+---+---+
// |   |   |   |   |   |   |   |   | 2
// +---+---+---+---+---+---+---+---+
//


//----------------------------------------------------------------------------------------------------------------------
AnimationFrameData::AnimationFrameData(
   const std::shared_ptr<sf::Texture>& texture,
   const sf::Vector2f& origin,
   uint32_t frame_width,
   uint32_t frame_height,
   uint32_t frame_count,
   uint32_t frames_per_row,
   const std::vector<sf::Time>& frame_times,
   uint32_t start_frame
)
   : _texture(texture),
     _origin(origin),
     _frame_times(frame_times)
{
   for (auto i = start_frame; i < frame_count + start_frame; i++)
   {
      const auto x = (i % frames_per_row);
      const auto y = static_cast<int32_t>((floor(static_cast<float>(start_frame + i) / frames_per_row)));

      // std::cout << this << " x: " << x << " y: " << y << std::endl;

      _frames.push_back(
         sf::IntRect(
            x * frame_width,
            y * frame_height,
            frame_width,
            frame_height
         )
      );
   }
}

