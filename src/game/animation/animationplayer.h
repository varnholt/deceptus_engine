#pragma once

#include "animation.h"

#include <vector>

#include <SFML/Graphics.hpp>

class AnimationPlayer
{
public:
   AnimationPlayer() = default;

   void add(const std::shared_ptr<Animation>& animation);
   void add(const std::vector<std::shared_ptr<Animation>>& animations);
   void update(const sf::Time& dt);
   void draw(sf::RenderTarget& target);

   static AnimationPlayer& getInstance();

private:
   std::vector<std::shared_ptr<Animation>> _animations;
};
