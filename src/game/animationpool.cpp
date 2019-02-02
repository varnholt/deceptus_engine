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
   const std::string& animationName,
   float x,
   float y,
   bool autoPlay,
   bool autoDelete
)
{
   if (mSettings.empty())
   {
      std::cerr << "initialize animation pool first!" << std::endl;
      return nullptr;
   }

   const auto& settings = mSettings[animationName];
   auto animation = std::make_shared<Animation>();

   animation->setOrigin(settings->mOrigin[0], settings->mOrigin[1]);
   animation->setPosition(x, y);

   animation->mName = animationName;
   animation->mFrames = settings->mFrames;
   animation->mTexture = settings->mTexture;
   animation->mFrameTime = settings->mFrameDuration;
   animation->mAutoDelete = autoDelete;

   if (autoPlay)
   {
      animation->play();
   }
   else
   {
      animation->pause();
   }

   mAnimations.push_back(animation);

   return animation;
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
      animation->update(sf::seconds(dt));
   }

   mAnimations.erase(
      std::remove_if(
         mAnimations.begin(), mAnimations.end(), [](const std::shared_ptr<Animation>& animation)
         {
            return (animation->mPaused && animation->mAutoDelete);
         }
      ),
      mAnimations.end()
   );
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


