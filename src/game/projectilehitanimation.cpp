#include "projectilehitanimation.h"


//----------------------------------------------------------------------------------------------------------------------
bool ProjectileHitAnimation::_initialized_default_animation = false;
std::unique_ptr<ProjectileHitAnimation::FrameData> ProjectileHitAnimation::_frame_data;
std::list<ProjectileHitAnimation*> ProjectileHitAnimation::_animations;


namespace
{
const auto width = 32;
const auto height = 32;
const auto sprites = 6;
const auto frameTime = 0.075f;
const sf::Time animationDuration = sf::milliseconds(400);
}


//----------------------------------------------------------------------------------------------------------------------
ProjectileHitAnimation::ProjectileHitAnimation()
{
   if (!_initialized_default_animation)
   {
      initialize();
   }

   setOrigin(width / 2, height / 2);
}


//----------------------------------------------------------------------------------------------------------------------
ProjectileHitAnimation::FrameData::FrameData(
   const std::shared_ptr<sf::Texture>& texture,
   uint32_t frame_width,
   uint32_t frame_height,
   uint32_t sprite_count,
   uint32_t sprites_per_row,
   const std::vector<sf::Time>& frame_times
)
   : _texture(texture),
     _frame_times(frame_times)
{
   for (auto i = 0u; i < sprite_count; i++)
   {
      _frames.push_back(
         sf::IntRect(
            i * frame_width,
            (i % sprites_per_row) * frame_height,
            frame_width,
            frame_height
         )
      );
   }
}



//----------------------------------------------------------------------------------------------------------------------
void ProjectileHitAnimation::initialize()
{
   auto texture = std::make_shared<sf::Texture>();
   if (texture->loadFromFile("data/weapons/detonation_big.png"))
   {
      std::vector<sf::Time> frame_times;
      for (auto i = 0u; i < sprites; i++)
      {
         frame_times.push_back(sf::seconds(frameTime));
      }

      _frame_data = std::make_unique<FrameData>(texture, width, height, sprites, sprites, frame_times);
      _initialized_default_animation = true;
   }
   else
   {
      printf("failed to load spritesheet!\n");
   }
}


//----------------------------------------------------------------------------------------------------------------------
void ProjectileHitAnimation::add(float x, float y)
{
   auto anim = new ProjectileHitAnimation();

   anim->mFrames = _frame_data->_frames;
   anim->mTexture = _frame_data->_texture;
   anim->mFrameTimes = _frame_data->_frame_times;

   anim->setPosition(x, y);
   anim->play();

   _animations.push_back(anim);
}


//----------------------------------------------------------------------------------------------------------------------
void ProjectileHitAnimation::add(float x, float y, const ProjectileHitAnimation::FrameData& frames)
{
   auto anim = new ProjectileHitAnimation();

   anim->mFrames = frames._frames;
   anim->mTexture = frames._texture;
   anim->mFrameTimes = frames._frame_times;

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
      ProjectileHitAnimation* sprite = (*it);

      if (sprite->mElapsed > animationDuration)
      {
         delete *it;
         _animations.erase(it++);
      }
      else
      {
         it++;
         sprite->update(dt);
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
std::list<ProjectileHitAnimation*>& ProjectileHitAnimation::getAnimations()
{
   return _animations;
}


