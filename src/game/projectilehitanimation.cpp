#include "projectilehitanimation.h"

#include "texturepool.h"

#include <cmath>
#include <iostream>


//----------------------------------------------------------------------------------------------------------------------
std::vector<ProjectileHitAnimation*> ProjectileHitAnimation::_active_animations;
std::map<std::string, AnimationFrameData> ProjectileHitAnimation::_reference_animations;


namespace
{
const auto width = 33;
const auto height = 32;
const auto sprites = 6;
const auto frame_time = 0.075f;
}


//----------------------------------------------------------------------------------------------------------------------
void ProjectileHitAnimation::playHitAnimation(float x, float y, float angle, const AnimationFrameData& frames)
{
   auto anim = new ProjectileHitAnimation();

   anim->_frames = frames._frames;
   anim->_color_texture = frames._texture;
   anim->setFrameTimes(frames._frame_times);
   anim->setOrigin(frames._origin);
   anim->setPosition(x, y);
   anim->setRotation(RADTODEG * angle);

   // stay at the last frame when animation is elapsed
   anim->_reset_to_first_frame = false;

   // can't simply play an animation, need to set a start frame!
   anim->updateVertices(0);

   // std::cout << "setting animation rotation to " << angle << std::endl;

   anim->play();

   _active_animations.push_back(anim);
}


//----------------------------------------------------------------------------------------------------------------------
void ProjectileHitAnimation::updateHitAnimations(const sf::Time& dt)
{
   std::vector<ProjectileHitAnimation*>::iterator it;
   for (it = _active_animations.begin(); it != _active_animations.end();)
   {
      auto animation = (*it);

      // after one loop animation will go into paused state
      if (animation->_paused)
      {
         // std::cout << "removing animation after " << animation->mElapsed.asMilliseconds() << "ms" << std::endl;
         delete animation;
         it = _active_animations.erase(it);
      }
      else
      {
         animation->update(dt);
         it++;
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
std::vector<ProjectileHitAnimation*>& ProjectileHitAnimation::getHitAnimations()
{
   return _active_animations;
}


//----------------------------------------------------------------------------------------------------------------------
void ProjectileHitAnimation::addReferenceAnimation(
   const std::string& id,
   const AnimationFrameData& animation
)
{
   _reference_animations.emplace(id, animation);
}


//----------------------------------------------------------------------------------------------------------------------
void ProjectileHitAnimation::addReferenceAnimation(
   const std::filesystem::path& texture_path,
   uint32_t frame_width,
   uint32_t frame_height,
   const std::chrono::duration<float, std::chrono::seconds::period>& time_per_frame,
   uint32_t frame_count,
   uint32_t frames_per_row,
   uint32_t start_frame
)
{
   auto texture = TexturePool::getInstance().get(texture_path);

   std::vector<sf::Time> frame_times;
   for (auto i = 0u; i < frame_count; i++)
   {
      frame_times.push_back(sf::seconds(time_per_frame.count()));
   }

   sf::Vector2f origin(
      static_cast<float_t>(frame_width / 2),
      static_cast<float_t>(frame_height / 2)
   );

   addReferenceAnimation(
      texture_path.string(),
      AnimationFrameData{
         texture,
         origin,
         frame_width,
         frame_height,
         frame_count,
         frames_per_row,
         frame_times,
         start_frame
      }
   );
}


//----------------------------------------------------------------------------------------------------------------------
AnimationFrameData ProjectileHitAnimation::getDefaultAnimation()
{
   const auto& texture = TexturePool::getInstance().get("data/weapons/detonation_big.png");

   std::vector<sf::Time> frame_times;
   for (auto i = 0u; i < sprites; i++)
   {
      frame_times.push_back(sf::seconds(frame_time));
   }

   sf::Vector2f origin(width / 2, height / 2);

   return AnimationFrameData{texture, origin, width, height, sprites, sprites, frame_times};
}


//----------------------------------------------------------------------------------------------------------------------
std::map<std::string, AnimationFrameData>::const_iterator ProjectileHitAnimation::getReferenceAnimation(const std::string& id)
{
   return _reference_animations.find(id);
}


//----------------------------------------------------------------------------------------------------------------------
void ProjectileHitAnimation::setupDefaultAnimation()
{
   // have a default animation in case there are none yet
   const auto it = _reference_animations.find("default");
   if (it == _reference_animations.end())
   {
      _reference_animations.emplace("default", getDefaultAnimation());
   }
}


