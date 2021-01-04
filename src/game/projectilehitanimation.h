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
         const sf::Vector2f& origin,
         uint32_t frame_width,
         uint32_t frame_height,
         uint32_t frame_count,
         uint32_t frames_per_row,
         const std::vector<sf::Time>& frame_times,
         uint32_t start_frame = 0
      );

      std::shared_ptr<sf::Texture> _texture;
      sf::Vector2f _origin;
      std::vector<sf::IntRect> _frames;
      std::vector<sf::Time> _frame_times;
   };

   ProjectileHitAnimation() = default;

   static void initialize();
   static void add(float x, float y, float angle, const FrameData& frames);
   static void updateAnimations(const sf::Time& dt);

   static std::list<ProjectileHitAnimation*>& getAnimations();

   static FrameData getDefaultAnimation();


protected:

   static std::list<ProjectileHitAnimation*> _animations;
};

