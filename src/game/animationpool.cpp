#include "animationpool.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

#include "json/json.hpp"
#include "texturepool.h"

using json = nlohmann::json;


AnimationPool AnimationPool::_player_animation;


//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::initialize()
{
   deserializeFromFile();
   _initialized = true;
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
   if (!_initialized)
   {
      initialize();
   }

   const auto& settings = _settings[name];
   auto animation = std::make_shared<Animation>();

   animation->setOrigin(settings->mOrigin[0], settings->mOrigin[1]);
   animation->setPosition(x, y);

   animation->_name = name;
   animation->_fames = settings->mFrames;
   animation->_texture_map = settings->mTexture;
   animation->_normal_map = settings->mNormalMap;
   animation->setFrameTimes(settings->mFrameDurations);

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
      _animations[name] = animation;
   }

   return animation;
}


//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::drawAnimations(
   sf::RenderTarget& color,
   sf::RenderTarget& normal,
   const std::vector<std::string>& animations
)
{
   for (const auto& key : animations)
   {
      const auto& animation = _animations.find(key);

      if (animation != _animations.end())
      {
         animation->second->draw(color, normal);
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::updateAnimations(const sf::Time& dt)
{
   if (_settings.empty())
   {
      return;
   }

   for (auto& animation : _animations)
   {
      animation.second->update(dt);
   }

   for (auto it = _animations.begin(); it != _animations.end();)
   {
      if (it->second->_paused == true && !it->second->_looped)
      {
         it = _animations.erase(it);
      }
      else
      {
         ++it;
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
const std::map<std::string, std::shared_ptr<Animation>>& AnimationPool::getAnimations()
{
   return _animations;
}


//----------------------------------------------------------------------------------------------------------------------
AnimationPool&AnimationPool::getInstance()
{
   return _player_animation;
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
         _settings[name] = settings;

         auto texture = TexturePool::getInstance().get(settings->mTexturePath);
         settings->mTexture = texture;

         const auto normal_map_filename = (settings->mTexturePath.stem().string() + "_normals" + settings->mTexturePath.extension().string());
         const auto normal_map_path = (settings->mTexturePath.parent_path() / normal_map_filename);

         if (std::filesystem::exists(normal_map_path))
         {
            auto normal_map = TexturePool::getInstance().get(normal_map_path);
            settings->mNormalMap = normal_map;
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


