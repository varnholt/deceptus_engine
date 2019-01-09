#pragma once

#include "game/sfmlanimatedsprite.h"

#include <array>
#include <list>
#include <memory>


class PlayerAnimation : public SfmlAnimatedSprite
{
   public:

      enum class PlayerAnimationType
      {
         Invalid = 0,
         JumpLeftAligned,
         JumpRightAligned,
         Size
      };

   private:

      struct PlayerAnimationSetup
      {
         int mWidth = 0;
         int mHeight = 0;
         int mSprites = 0;
         float mOriginX = 0.0f;
         float mOriginY = 0.0f;
         float mFrameTime = 0.0f;
         int mAnimationDuration = 0;
         sf::Texture mTexture;
         SfmlAnimation mAnimation;
      };

      static bool sInitialized;
      static std::array<std::shared_ptr<PlayerAnimationSetup>, static_cast<size_t>(PlayerAnimationType::Size)> sSetups;
      static std::list<PlayerAnimation*> sAnimations;
      static std::list<PlayerAnimation*> sElapsedAnimations;


   public:

      static void initialize();
      static void add(PlayerAnimationType type, float x, float y);
      static void updateAnimations(float dt);
      static std::list<PlayerAnimation*>* getAnimations();


   private:

      PlayerAnimation(PlayerAnimationType type, sf::Time mFrameTime = sf::seconds(0.2f));
      PlayerAnimationType mType;
};

