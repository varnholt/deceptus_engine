#include "projectilehitanimation.h"


//----------------------------------------------------------------------------------------------------------------------
bool ProjectileHitAnimation::sInitialized = false;
std::shared_ptr<sf::Texture> ProjectileHitAnimation::sTexture;
std::vector<sf::IntRect> ProjectileHitAnimation::sFrames;
std::list<ProjectileHitAnimation*> ProjectileHitAnimation::sAnimations;
std::list<ProjectileHitAnimation*> ProjectileHitAnimation::sElapsedAnimations;

const auto width = 32;
const auto height = 32;
const auto sprites = 6;
const auto frameTime = 0.075f;
const sf::Time animationDuration = sf::milliseconds(400);


//----------------------------------------------------------------------------------------------------------------------
ProjectileHitAnimation::ProjectileHitAnimation()
{
   if (!sInitialized)
   {
      initialize();
   }

   for (auto i = 0u; i < sFrames.size(); i++)
   {
      mFrameTimes.push_back(sf::seconds(frameTime));
   }

   setOrigin(width / 2, height / 2);
}


//----------------------------------------------------------------------------------------------------------------------
void ProjectileHitAnimation::initialize()
{
   sTexture = std::make_shared<sf::Texture>();
   if (sTexture->loadFromFile("data/weapons/detonation_big.png"))
   {
      for (int i = 0; i < sprites; i++)
      {
         sFrames.push_back(sf::IntRect(i * (width + 1), 0, width, height));
      }

      sInitialized = true;
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

   anim->mFrames = sFrames;
   anim->mTexture = sTexture;

   anim->setPosition(x, y);
   anim->play();

   sAnimations.push_back(anim);
}


//----------------------------------------------------------------------------------------------------------------------
void ProjectileHitAnimation::updateAnimations(const sf::Time& dt)
{
   std::list<ProjectileHitAnimation*>::iterator it;
   for (it = sAnimations.begin(); it != sAnimations.end();)
   {
      ProjectileHitAnimation* sprite = (*it);

      if (sprite->mElapsed > animationDuration)
      {
         delete *it;
         sAnimations.erase(it++);
      }
      else
      {
         it++;
         sprite->update(dt);
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
std::list<ProjectileHitAnimation*> *ProjectileHitAnimation::getAnimations()
{
   return &sAnimations;
}


