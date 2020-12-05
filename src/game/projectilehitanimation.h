#pragma once

#include "game/animation.h"

#include <list>


class ProjectileHitAnimation : public Animation
{
public:

   struct FrameData
   {
      FrameData() = default;
      FrameData(
         const std::shared_ptr<sf::Texture>& texture,
         uint32_t frame_width,
         uint32_t frame_height,
         uint32_t sprite_count,
         uint32_t sprites_per_row,
         const std::vector<sf::Time>& frame_times
      );

      std::shared_ptr<sf::Texture> _texture;
      std::vector<sf::IntRect> _frames;
      std::vector<sf::Time> _frame_times;
   };

   ProjectileHitAnimation();

   static void initialize();
   static void add(float x, float y, const FrameData& frames);
   static void updateAnimations(const sf::Time& dt);

   static std::list<ProjectileHitAnimation*>& getAnimations();

   static FrameData getDefaultAnimation();


protected:

   static std::list<ProjectileHitAnimation*> _animations;
};

