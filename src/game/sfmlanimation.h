#pragma once

#include <vector>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Texture.hpp>

struct Animation
{
   Animation() = default;
   std::vector<sf::IntRect> mFrames;
   sf::Texture mTexture;
};
