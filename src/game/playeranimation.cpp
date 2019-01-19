#include "playeranimation.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

#include "json/json.hpp"

using json = nlohmann::json;


PlayerAnimation PlayerAnimation::sPlayerAnimation;


//----------------------------------------------------------------------------------------------------------------------
void PlayerAnimation::initialize()
{
   // jump dust left aligned
   auto dustLeft = std::make_shared<PlayerAnimationSetup>();
   dustLeft->mWidth = 24;
   dustLeft->mHeight = 24;
   dustLeft->mSprites = 6;
   dustLeft->mFrameTime = 0.075f;
   dustLeft->mAnimationDuration = 400;
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
   auto dustRight = std::make_shared<PlayerAnimationSetup>();
   dustRight->mWidth = 24;
   dustRight->mHeight = 24;
   dustRight->mSprites = 6;
   dustRight->mFrameTime = 0.075f;
   dustRight->mAnimationDuration = 400;
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
void PlayerAnimation::add(AnimationType type, float x, float y)
{
   if (!sInitialized)
   {
      initialize();
   }

   auto setup = mSetups[type];
   auto anim = new SpriteAnimation();

   anim->setOrigin(setup->mOriginX, setup->mOriginY);
   anim->mType = type;
   anim->setPosition(x, y);
   anim->mFrames = setup->mFrames;
   anim->mTexture = setup->mTexture;
   anim->play();

   mAnimations.push_back(anim);
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerAnimation::updateAnimations(float dt)
{
   if (!sInitialized)
   {
     return;
   }

   mAnimations.erase(
      std::remove_if(
         mAnimations.begin(), mAnimations.end(), [this](SpriteAnimation* animation)
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
      animation->update(dt);
      animation->incrementElapsed(static_cast<int>(dt * 1000.0f));
   }
}


//----------------------------------------------------------------------------------------------------------------------
const std::vector<SpriteAnimation*>& PlayerAnimation::getAnimations()
{
   return mAnimations;
}


//----------------------------------------------------------------------------------------------------------------------
PlayerAnimation&PlayerAnimation::getInstance()
{
   return sPlayerAnimation;
}



void PlayerAnimation::deserialize(const std::string& data)
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


