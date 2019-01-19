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
   auto dustLeft = std::make_shared<AnimationSettings>();
   dustLeft->mWidth = 24;
   dustLeft->mHeight = 24;
   dustLeft->mSprites = 6;
   dustLeft->mFrameTime = sf::seconds(0.075f);
   dustLeft->mAnimationDuration = sf::milliseconds(400);
   dustLeft->mOriginX = 9.0f;
   dustLeft->mOriginY = 12.0f;
   dustLeft->mTexture.loadFromFile("data/sprites/player.png");

   for (int i = 0; i < dustLeft->mSprites; i++)
   {
      dustLeft->mFrames.push_back(
         sf::IntRect(
            i * (dustLeft->mWidth + 1),
            8 * 24,
            dustLeft->mWidth,
            dustLeft->mHeight
         )
      );
   }

   // jump dust right aligned
   auto dustRight = std::make_shared<AnimationSettings>();
   dustRight->mWidth = 24;
   dustRight->mHeight = 24;
   dustRight->mSprites = 6;
   dustRight->mFrameTime = sf::seconds(0.075f);
   dustRight->mAnimationDuration = sf::milliseconds(400);
   dustRight->mOriginX = 12.0f;
   dustRight->mOriginY = 12.0f;
   dustRight->mTexture.loadFromFile("data/sprites/player.png");

   for (int i = 0; i < dustRight->mSprites; i++)
   {
      dustRight->mFrames.push_back(
         sf::IntRect(
            i * (dustRight->mWidth + 1),
            8 * 24,
            dustRight->mWidth,
            dustRight->mHeight
         )
      );
   }

   mSetups[AnimationType::JumpDustLeftAligned] = dustLeft;
   mSetups[AnimationType::JumpDustRightAligned] = dustRight;

   sInitialized = true;
}


//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::add(AnimationType type, float x, float y)
{
   if (!sInitialized)
   {
      initialize();
   }

   auto setup = mSetups[type];
   auto anim = new Animation();

   anim->setOrigin(setup->mOriginX, setup->mOriginY);
   anim->mType = type;
   anim->setPosition(x, y);
   anim->mFrames = setup->mFrames;
   anim->mTexture = setup->mTexture;
   anim->mFrameTime = setup->mFrameTime;
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
         mAnimations.begin(), mAnimations.end(), [this](Animation* animation)
         {
            if (animation->mType == AnimationType::Invalid)
            {
               return false;
            }
            auto setup = mSetups[animation->mType];
            return (animation->mElapsed > setup->mAnimationDuration);
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
const std::vector<Animation*>& AnimationPool::getAnimations()
{
   return mAnimations;
}


//----------------------------------------------------------------------------------------------------------------------
AnimationPool&AnimationPool::getInstance()
{
   return sPlayerAnimation;
}



void AnimationPool::deserialize(const std::string& data)
{
   json config = json::parse(data);

   try
   {
     // mLevels = config.get<std::vector<LevelItem>>();
   }
   catch (const std::exception& e)
   {
     std::cout << e.what() << std::endl;
   }
}


