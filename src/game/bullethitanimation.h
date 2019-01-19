#pragma once

#include "game/sfmlanimatedsprite.h"

#include <list>


class BulletHitAnimation : public SpriteAnimation
{
   protected:

      static bool sInitialized;

      static sf::Texture sTexture;
      static std::vector<sf::IntRect> sFrames;


      static std::list<BulletHitAnimation*> sAnimations;
      static std::list<BulletHitAnimation*> sElapsedAnimations;


   public:

      BulletHitAnimation(sf::Time mFrameTime = sf::seconds(0.2f));

      static void initialize();
      static void add(float x, float y);
      static void updateAnimations(float dt);
      static std::list<BulletHitAnimation*>* getAnimations();
};

