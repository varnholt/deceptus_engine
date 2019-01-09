#include "playeranimation.h"


//----------------------------------------------------------------------------------------------------------------------
bool PlayerAnimation::sInitialized = false;
std::list<PlayerAnimation*> PlayerAnimation::sAnimations;
std::list<PlayerAnimation*> PlayerAnimation::sElapsedAnimations;
std::array<std::shared_ptr<PlayerAnimation::PlayerAnimationSetup>, static_cast<size_t>(PlayerAnimation::PlayerAnimationType::Size)> PlayerAnimation::sSetups;


//----------------------------------------------------------------------------------------------------------------------
PlayerAnimation::PlayerAnimation(PlayerAnimationType type, sf::Time frameTime)
 : SfmlAnimatedSprite(frameTime),
   mType(type)
{
   auto setup = sSetups[static_cast<size_t>(mType)];
   setOrigin(setup->mOriginX, setup->mOriginY);
   setAnimation(setup->mAnimation);
}


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
   dustLeft->mAnimation.setSpriteSheet(dustLeft->mTexture);

   for (int i = 0; i < dustLeft->mSprites; i++)
   {
      dustLeft->mAnimation.addFrame(sf::IntRect(i * (dustLeft->mWidth + 1), 8 * 24, dustLeft->mWidth, dustLeft->mHeight));
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
   dustRight->mAnimation.setSpriteSheet(dustRight->mTexture);

   for (int i = 0; i < dustRight->mSprites; i++)
   {
      dustRight->mAnimation.addFrame(sf::IntRect(i * (dustRight->mWidth + 1), 8 * 24, dustRight->mWidth, dustRight->mHeight));
   }

   sSetups[static_cast<size_t>(PlayerAnimationType::JumpLeftAligned)] = dustLeft;
   sSetups[static_cast<size_t>(PlayerAnimationType::JumpRightAligned)] = dustRight;

   sInitialized = true;
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerAnimation::add(PlayerAnimationType type, float x, float y)
{
   if (!sInitialized)
   {
      initialize();
   }

   auto setup = sSetups[static_cast<size_t>(type)];
   auto anim = new PlayerAnimation(type, sf::seconds(setup->mFrameTime));
   anim->setPosition(x, y);
   anim->play();

   sAnimations.push_back(anim);
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerAnimation::updateAnimations(float dt)
{
   if (!sInitialized)
   {
     return;
   }

   std::list<PlayerAnimation*>::iterator it;
   for (it = sAnimations.begin(); it != sAnimations.end();)
   {
      PlayerAnimation* sprite = (*it);
      auto setup = sSetups[static_cast<size_t>(sprite->mType)];

      if (sprite->getElapsed() > setup->mAnimationDuration)
      {
         delete *it;
         sAnimations.erase(it++);
      }
      else
      {
         it++;
         sprite->update(dt);
         sprite->incrementElapsed(static_cast<int>(dt * 1000.0f));
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
std::list<PlayerAnimation*> *PlayerAnimation::getAnimations()
{
   return &sAnimations;
}


