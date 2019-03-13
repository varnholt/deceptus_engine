#include "animationsettings.h"


void from_json(const nlohmann::json& j, AnimationSettings& settings)
{
   auto spriteCount = j.at("sprite_count").get<int32_t>();

   settings.mFrameSize = j.at("frame_size").get<std::array<int32_t, 2>>();
   settings.mFrameOffset = j.at("frame_offset").get<std::array<int32_t, 2>>();
   settings.mOrigin = j.at("origin").get<std::array<float, 2>>();
   settings.mTexturePath = j.at("texture").get<std::string>();

   auto frameDurations = j.at("frame_durations").get<std::vector<int32_t>>();

   // if user just enters a single frame duration, just duplicate that for each sprite
   if (frameDurations.size() == 1)
   {
      for (auto i = 0; i < spriteCount; i++)
      {
        settings.mFrameDurations.push_back(sf::milliseconds(frameDurations.at(0)));
      }
   }
   else
   {
      // otherwise have one frame duration for each frame
      for (const auto duration : frameDurations)
      {
        settings.mFrameDurations.push_back(sf::milliseconds(duration));
      }
   }

   for (auto i = 0; i < spriteCount; i++)
   {
      settings.mFrames.push_back(
         sf::IntRect(
            settings.mFrameOffset[0] + (i * settings.mFrameSize[0]),
            settings.mFrameOffset[1],
            settings.mFrameSize[0],
            settings.mFrameSize[1]
         )
      );
   }
}

