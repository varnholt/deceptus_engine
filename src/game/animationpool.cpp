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
   deserializeFromFile();
}


//----------------------------------------------------------------------------------------------------------------------
std::shared_ptr<Animation> AnimationPool::add(
   const std::string& name,
   float x,
   float y,
   bool autoPlay,
   bool managedByPool
)
{
   if (mSettings.empty())
   {
      std::cerr << "initialize animation pool first!" << std::endl;
      return nullptr;
   }

   const auto& settings = mSettings[name];
   auto animation = std::make_shared<Animation>();

   animation->setOrigin(settings->mOrigin[0], settings->mOrigin[1]);
   animation->setPosition(x, y);

   animation->mName = name;
   animation->mFrames = settings->mFrames;
   animation->mTexture = settings->mTexture;
   animation->mFrameTime = settings->mFrameDuration;

   if (autoPlay)
   {
      animation->play();
   }
   else
   {
      animation->pause();
   }

   if (managedByPool)
   {
      mAnimations[name] = animation;
   }

   return animation;
}


//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::drawAnimations(
   sf::RenderTarget& target,
   const std::vector<std::string>& animations
)
{
   for (const auto& key : animations)
   {
      const auto& animation = mAnimations.find(key);

      if (animation != mAnimations.end())
      {
         animation->second->draw(target);
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::updateAnimations(float dt)
{
   if (mSettings.empty())
   {
      return;
   }

   for (auto animation : mAnimations)
   {
      animation.second->update(sf::seconds(dt));
   }

   for (auto it = mAnimations.begin(); it != mAnimations.end();)
   {
       if (it->second->mPaused == true && !it->second->mLooped)
          it = mAnimations.erase(it);
       else
          ++it;
   }
}


//----------------------------------------------------------------------------------------------------------------------
const std::map<std::string, std::shared_ptr<Animation>>& AnimationPool::getAnimations()
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
      for (auto& item : config.get<json::object_t>())
      {
         auto name = item.first;
         auto settings = std::make_shared<AnimationSettings>(item.second.get<AnimationSettings>());
         mSettings[name] = settings;

         // use a single pool for textures only
         auto textureIt = mTextures.find(settings->mTexturePath);
         if (textureIt != mTextures.end())
         {
            settings->mTexture = textureIt->second;
         }
         else
         {
            auto texture = std::make_shared<sf::Texture>();
            if (texture->loadFromFile(settings->mTexturePath))
            {
               mTextures[settings->mTexturePath] = texture;
               settings->mTexture = texture;
            }
            else
            {
               std::cerr << "AnimationPool::deserialize: texture '" << settings->mTexturePath << "' not found." << std::endl;
            }
         }
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


