#include "animationsettings.h"


void from_json(const nlohmann::json& j, AnimationSettings& settings)
{
   auto spriteCount = j.at("sprite_count").get<int32_t>();
   auto texture = j.at("texture").get<std::string>();

   settings.mFrameSize = j.at("frame_size").get<std::array<int32_t, 2>>();
   settings.mFrameOffset = j.at("frame_offset").get<std::array<int32_t, 2>>();
   settings.mFrameDuration = sf::milliseconds(j.at("frame_duration").get<int32_t>());
   settings.mOrigin = j.at("origin").get<std::array<float, 2>>();
   settings.mTexture.loadFromFile(texture);

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

