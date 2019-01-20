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
void AnimationPool::add(const std::string animationName, float x, float y)
{
   if (mSettings.empty())
   {
      std::cerr << "initialize animation pool first!" << std::endl;
      return;
   }

   const auto& setup = mSettings[animationName];
   auto anim = std::make_shared<Animation>();

   anim->setOrigin(setup.mOrigin[0], setup.mOrigin[1]);
   anim->mName = animationName;
   anim->setPosition(x, y);
   anim->mFrames = setup.mFrames;
   anim->mTexture = setup.mTexture;
   anim->mFrameTime = setup.mFrameDuration;
   anim->play();

   mAnimations.push_back(anim);
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
            return (animation->mPaused);
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
         AnimationSettings settings = item.second.get<AnimationSettings>();
         mSettings[name] = settings;
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


