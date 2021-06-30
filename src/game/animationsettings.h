#pragma once

#include <SFML/Graphics.hpp>

#include "json/json.hpp"

#include <cstdint>
#include <filesystem>
#include <stdint.h>
#include <vector>


struct AnimationSettings
{
   std::array<int32_t, 2> mFrameSize;
   std::array<int32_t, 2> mFrameOffset;
   std::array<float, 2> mOrigin;
   std::vector<sf::Time> mFrameDurations;
   std::filesystem::path mTexturePath;
   std::shared_ptr<sf::Texture> mTexture;
   std::shared_ptr<sf::Texture> mNormalMap;
   std::vector<sf::IntRect> mFrames;

   AnimationSettings() = default;

   AnimationSettings(
      const std::array<int32_t, 2>& frameSize,
      const std::array<int32_t, 2>& frameOffset,
      const std::array<float, 2>& origin,
      const std::vector<sf::Time>& frameDurations,
      const std::filesystem::path& texturePath
   );
};

void from_json(const nlohmann::json& j, AnimationSettings& settings);
void to_json(nlohmann::json& j, const AnimationSettings& settings);
