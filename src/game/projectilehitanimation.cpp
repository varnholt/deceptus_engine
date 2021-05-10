#include "projectilehitanimation.h"

#include "texturepool.h"

#include <cmath>
#include <iostream>


//----------------------------------------------------------------------------------------------------------------------
std::list<ProjectileHitAnimation*> ProjectileHitAnimation::_animations;


namespace
{
const auto width = 33;
const auto height = 32;
const auto sprites = 6;
const auto frame_time = 0.075f;
}


//----------------------------------------------------------------------------------------------------------------------
void ProjectileHitAnimation::add(float x, float y, float angle, const AnimationFrameData& frames)
{
   auto anim = new ProjectileHitAnimation();

   anim->_fames = frames._frames;
   anim->_texture_map = frames._texture;
   anim->setFrameTimes(frames._frame_times);
   anim->setOrigin(frames._origin);
   anim->setPosition(x, y);
   anim->setRotation(RADTODEG * angle);

   // stay at the last frame when animation is elapsed
   anim->_reset_to_first_frame = false;

   // can't simply play an animation, need to set a start frame!
   anim->setFrame(0);

   // std::cout << "setting animation rotation to " << angle << std::endl;

   anim->play();

   _animations.push_back(anim);
}


//----------------------------------------------------------------------------------------------------------------------
void ProjectileHitAnimation::updateAnimations(const sf::Time& dt)
{
   std::list<ProjectileHitAnimation*>::iterator it;
   for (it = _animations.begin(); it != _animations.end();)
   {
      auto animation = (*it);

      // after one loop animation will go into paused state
      if (animation->_paused)
      {
         // std::cout << "removing animation after " << animation->mElapsed.asMilliseconds() << "ms" << std::endl;
         delete animation;
         it = _animations.erase(it);
      }
      else
      {
         animation->update(dt);
         it++;
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
std::list<ProjectileHitAnimation*>& ProjectileHitAnimation::getAnimations()
{
   return _animations;
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


