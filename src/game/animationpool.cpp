#include "animationpool.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

#include "json/json.hpp"

using json = nlohmann::json;


AnimationPool AnimationPool::sPlayerAnimation;


//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::initialize()
{
   // jump dust left aligned
   AnimationSettings dustLeft;
   dustLeft.mWidth = 24;
   dustLeft.mHeight = 24;
   dustLeft.mSprites = 6;
   dustLeft.mFrameTime = sf::seconds(0.075f);
   dustLeft.mAnimationDuration = sf::milliseconds(400);
   dustLeft.mOriginX = 9.0f;
   dustLeft.mOriginY = 12.0f;
   dustLeft.mTexture.loadFromFile("data/sprites/player.png");

   for (int i = 0; i < dustLeft.mSprites; i++)
   {
      dustLeft.mFrames.push_back(
         sf::IntRect(
            i * (dustLeft.mWidth + 1),
            8 * 24,
            dustLeft.mWidth,
            dustLeft.mHeight
         )
      );
   }

   // jump dust right aligned
   AnimationSettings dustRight;
   dustRight.mWidth = 24;
   dustRight.mHeight = 24;
   dustRight.mSprites = 6;
   dustRight.mFrameTime = sf::seconds(0.075f);
   dustRight.mAnimationDuration = sf::milliseconds(400);
   dustRight.mOriginX = 12.0f;
   dustRight.mOriginY = 12.0f;
   dustRight.mTexture.loadFromFile("data/sprites/player.png");

   for (int i = 0; i < dustRight.mSprites; i++)
   {
      dustRight.mFrames.push_back(
         sf::IntRect(
            i * (dustRight.mWidth + 1),
            8 * 24,
            dustRight.mWidth,
            dustRight.mHeight
         )
      );
   }

   mSettings[AnimationType::PlayerJumpDustLeftAligned] = dustLeft;
   mSettings[AnimationType::PlayerJumpDustRightAligned] = dustRight;

   sInitialized = true;
}


//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::add(AnimationType type, float x, float y)
{
   if (!sInitialized)
   {
      initialize();
   }

   const auto& setup = mSettings[type];
   auto anim = std::make_shared<Animation>();

   anim->setOrigin(setup.mOriginX, setup.mOriginY);
   anim->mType = type;
   anim->setPosition(x, y);
   anim->mFrames = setup.mFrames;
   anim->mTexture = setup.mTexture;
   anim->mFrameTime = setup.mFrameTime;
   anim->play();

   mAnimations.push_back(anim);
}


//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::updateAnimations(float dt)
{
   if (!sInitialized)
   {
     return;
   }

   mAnimations.erase(
      std::remove_if(
         mAnimations.begin(), mAnimations.end(), [this](const std::shared_ptr<Animation>& animation)
         {
            if (animation->mType == AnimationType::Invalid)
            {
               return false;
            }
            const auto& settings = mSettings[animation->mType];
            return (animation->mElapsed > settings.mAnimationDuration);
         }
      ),
      mAnimations.end()
   );

   for (auto animation : mAnimations)
   {
      animation->update(sf::seconds(dt));
   }
}


//----------------------------------------------------------------------------------------------------------------------
const std::vector<std::shared_ptr<Animation>>& AnimationPool::getAnimations()
{
   return mAnimations;
}


//----------------------------------------------------------------------------------------------------------------------
AnimationPool&AnimationPool::getInstance()
{
   return sPlayerAnimation;
}


//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::deserialize(const std::string& data)
{
   json config = json::parse(data);

   try
   {
      auto settings = config.get<std::vector<AnimationSettings>>();

      for (const auto& settings : settings)
      {

      }
   }
   catch (const std::exception& e)
   {
     std::cout << e.what() << std::endl;
   }
}


//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::deserializeFromFile(const std::string &filename)
{
  std::ifstream ifs (filename, std::ifstream::in);

  auto c = ifs.get();
  std::string data;

  while (ifs.good())
  {
    data.push_back(static_cast<char>(c));
    c = ifs.get();
  }

  ifs.close();

  deserialize(data);
}


