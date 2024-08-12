#pragma once

#include "game/animation/animation.h"
#include "game/animation/animationframedata.h"

#include <chrono>
#include <filesystem>
#include <list>
#include <optional>

class ProjectileHitAnimation : public Animation
{
public:
   ProjectileHitAnimation() = default;

   // active animations
   static void playHitAnimation(float x, float y, float angle, const AnimationFrameData& frames);
   static void updateHitAnimations(const sf::Time& dt);
   static std::vector<ProjectileHitAnimation*>& getHitAnimations();

   // reference animations
   static void addReferenceAnimation(const std::string& id, const AnimationFrameData& animation);
   static void addReferenceAnimation(
      const std::filesystem::path& texture_path,
      uint32_t frame_width,
      uint32_t frame_height,
      const std::chrono::duration<float, std::chrono::seconds::period>& time_per_frame,
      uint32_t frame_count,
      uint32_t frames_per_row,
      uint32_t start_frame
   );

   static std::map<std::string, AnimationFrameData>::const_iterator getReferenceAnimation(const std::string& id);
   static void setupDefaultAnimation();
   static AnimationFrameData getDefaultAnimation();

protected:
   static std::vector<ProjectileHitAnimation*> __active_animations;
   static std::map<std::string, AnimationFrameData> __reference_animations;
};
