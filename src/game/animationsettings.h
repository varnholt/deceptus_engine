#pragma once

#include <SFML/Graphics.hpp>

#include "json/json.hpp"

#include <stdint.h>
#include <vector>


struct AnimationSettings
{
   std::array<int32_t, 2> mFrameSize;
   std::array<int32_t, 2> mFrameOffset;
   std::array<float, 2> mOrigin;
   sf::Time mFrameDuration;
   std::string mTexturePath;
   std::shared_ptr<sf::Texture> mTexture;
   std::vector<sf::IntRect> mFrames;
};

void from_json(const nlohmann::json& j, AnimationSettings& settings);
