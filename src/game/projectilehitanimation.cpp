#include "projectilehitanimation.h"

#include "texturepool.h"

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


//   0   1   2   3   4   5   6   7
// +---+---+---+---+---+---+---+---+
// |   |   |   |   |   |###|###|###| 0
// +---+---+---+---+---+---+---+---+
// |###|###|###|   |   |   |   |   | 1
// +---+---+---+---+---+---+---+---+
// |   |   |   |   |   |   |   |   | 2
// +---+---+---+---+---+---+---+---+
//


//----------------------------------------------------------------------------------------------------------------------
ProjectileHitAnimation::FrameData::FrameData(
   const std::shared_ptr<sf::Texture>& texture,
   const sf::Vector2f& origin,
   uint32_t frame_width,
   uint32_t frame_height,
   uint32_t sprite_count,
   uint32_t sprites_per_row,
   const std::vector<sf::Time>& frame_times,
   uint32_t start_frame
)
   : _texture(texture),
     _origin(origin),
     _frame_times(frame_times)
{
   for (auto i = start_frame; i < sprite_count + start_frame; i++)
   {
      auto row = static_cast<int32_t>((floor(static_cast<float>(start_frame + i) / sprites_per_row)) * frame_height);

      _frames.push_back(
         sf::IntRect(
            i * frame_width,
            row,
            frame_width,
            frame_height
         )
      );
   }
}



//----------------------------------------------------------------------------------------------------------------------
void ProjectileHitAnimation::initialize()
{
}


//----------------------------------------------------------------------------------------------------------------------
void ProjectileHitAnimation::add(float x, float y, const ProjectileHitAnimation::FrameData& frames)
{
   auto anim = new ProjectileHitAnimation();

   anim->mFrames = frames._frames;
   anim->mTexture = frames._texture;
   anim->setFrameTimes(frames._frame_times);
   anim->setOrigin(frames._origin);
   anim->setPosition(x, y);
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

      // animation_duration
      if (animation->mElapsed > animation->mOverallTime)
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
ProjectileHitAnimation::FrameData ProjectileHitAnimation::getDefaultAnimation()
{
   auto texture = TexturePool::getInstance().get("data/weapons/detonation_big.png");

   std::vector<sf::Time> frame_times;
   for (auto i = 0u; i < sprites; i++)
   {
      frame_times.push_back(sf::seconds(frame_time));
   }

   sf::Vector2f origin(width / 2, height / 2);

   return FrameData{texture, origin, width, height, sprites, sprites, frame_times};
}


