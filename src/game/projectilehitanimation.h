#pragma once

#include "game/animation.h"

#include <list>


class ProjectileHitAnimation : public Animation
{
public:

   ProjectileHitAnimation();

   static void initialize();
   static void add(float x, float y);
   static void updateAnimations(const sf::Time& dt);
   static std::list<ProjectileHitAnimation*>* getAnimations();

protected:

   static bool sInitialized;

   static std::shared_ptr<sf::Texture> sTexture;
   static std::vector<sf::IntRect> sFrames;


   static std::list<ProjectileHitAnimation*> sAnimations;
   static std::list<ProjectileHitAnimation*> sElapsedAnimations;
};

