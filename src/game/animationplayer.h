#pragma once

#include "animation.h"

#include <vector>

#include <SFML/Graphics.hpp>


class AnimationPlayer
{

public:
   AnimationPlayer() = default;

   void add(const Animation& animation);
   void add(const std::vector<Animation>& animations);
   void update(const sf::Time& dt);
   void draw(sf::RenderTarget& target);

   static AnimationPlayer& getInstance();

private:

   std::vector<Animation> _animations;

};
