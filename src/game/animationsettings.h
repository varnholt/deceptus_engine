#pragma once

#include <SFML/Graphics.hpp>

#include "json/json.hpp"

#include <stdint.h>
#include <vector>


struct AnimationSettings
{
   int32_t mWidth = 0;
   int32_t mHeight = 0;
   int32_t mSprites = 0;
   float mOriginX = 0.0f;
   float mOriginY = 0.0f;
   sf::Time mFrameTime;
   sf::Time mAnimationDuration;
   sf::Texture mTexture;
   std::vector<sf::IntRect> mFrames;
};

void from_json(const nlohmann::json& j, AnimationSettings& settings);
