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
   sf::Texture mTexture; // to do: have these just once (so duplicated textures are not stored)
   std::vector<sf::IntRect> mFrames;
};

void from_json(const nlohmann::json& j, AnimationSettings& settings);
