#pragma once

#include "game/animation.h"

#include <list>


class BulletHitAnimation : public Animation
{
public:

   BulletHitAnimation();

   static void initialize();
   static void add(float x, float y);
   static void updateAnimations(float dt);
   static std::list<BulletHitAnimation*>* getAnimations();

protected:

   static bool sInitialized;

   static std::shared_ptr<sf::Texture> sTexture;
   static std::vector<sf::IntRect> sFrames;


   static std::list<BulletHitAnimation*> sAnimations;
   static std::list<BulletHitAnimation*> sElapsedAnimations;
};

