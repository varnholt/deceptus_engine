#pragma once

#include "game/animation/animation.h"
#include "game/animation/animationframedata.h"

#include <chrono>
#include <filesystem>
#include <list>
#include <map>
#include <optional>

/// \brief global manager for transient projectile impact animations.
class ProjectileHitAnimation : public Animation
{
public:
   /// \brief constructs an empty hit animation instance.
   ProjectileHitAnimation() = default;

   // active animations
   /// \brief creates and plays one impact animation instance at a world-space pixel position.
   /// \param x impact x position in pixels.
   /// \param y impact y position in pixels.
   /// \param angle impact rotation angle in radians.
   /// \param frames frame data copied into the spawned hit animation.
   static void playHitAnimation(float x, float y, float angle, const AnimationFrameData& frames);

   /// \brief advances active hit animations and destroys those that finished playback.
   /// \param dt frame delta time.
   static void updateHitAnimations(const sf::Time& dt);

   /// \brief returns all currently active hit animations.
   /// \return reference to the active animation pointer list.
   static std::vector<ProjectileHitAnimation*>& getHitAnimations();

   // reference animations
   /// \brief registers reference frame data used for impact lookup by identifier.
   /// \param id identifier key used by projectiles.
   /// \param animation reference frame data for this identifier.
   static void addReferenceAnimation(const std::string& id, const AnimationFrameData& animation);

   /// \brief builds and registers reference frame data from sprite-sheet parameters.
   /// \param texture_path path to the sprite-sheet texture.
   /// \param frame_width frame width in pixels.
   /// \param frame_height frame height in pixels.
   /// \param time_per_frame duration of each frame.
   /// \param frame_count number of frames to include.
   /// \param frames_per_row number of frames per sprite-sheet row.
   /// \param start_frame first frame index in the sheet.
   static void addReferenceAnimation(
      const std::filesystem::path& texture_path,
      int32_t frame_width,
      int32_t frame_height,
      const std::chrono::duration<float, std::chrono::seconds::period>& time_per_frame,
      int32_t frame_count,
      int32_t frames_per_row,
      int32_t start_frame
   );

   /// \brief finds reference frame data for an identifier.
   /// \param id identifier key previously registered.
   /// \return const iterator to the matching map entry, or end when missing.
   static std::map<std::string, AnimationFrameData>::const_iterator getReferenceAnimation(const std::string& id);

   /// \brief ensures a default reference animation exists under the "default" key.
   static void setupDefaultAnimation();

   /// \brief builds the built-in fallback impact animation frame data.
   /// \return default detonation-style animation frame data.
   static AnimationFrameData getDefaultAnimation();

protected:
   static std::vector<ProjectileHitAnimation*> __active_animations;
   static std::map<std::string, AnimationFrameData> __reference_animations;
};
