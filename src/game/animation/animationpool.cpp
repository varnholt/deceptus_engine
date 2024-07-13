#include "animationpool.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

#include "framework/tools/log.h"
#include "game/texturepool.h"

#include "json/json.hpp"

using json = nlohmann::json;

//----------------------------------------------------------------------------------------------------------------------
AnimationPool::AnimationPool(const std::string& file_path)
 : _file_path(file_path)
{
}

//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::initialize()
{
   if (!_initialized)
   {
      deserializeFromFile(_file_path);
      _initialized = true;
   }
}

//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::reload()
{
   _initialized = false;
   initialize();
}

//----------------------------------------------------------------------------------------------------------------------
std::shared_ptr<Animation> AnimationPool::create(const std::string& name, float x, float y, bool auto_play, bool managed_by_pool)
{
   if (!_initialized)
   {
      initialize();
   }

   const auto& settings = _settings[name];

   if (!settings)
   {
      Log::Error() << "animation '" << name << "' is not defined in json";
   }

   auto animation = std::make_shared<Animation>();

   animation->setOrigin(settings->_origin[0], settings->_origin[1]);
   animation->setPosition(x, y);

   animation->_name = name;
   animation->_frames = settings->_frames;
   animation->_color_texture = settings->_texture;
   animation->_normal_texture = settings->_normal_map;
   animation->setFrameTimes(settings->_frame_durations);

   if (auto_play)
   {
      animation->play();
   }
   else
   {
      animation->pause();
   }

   if (managed_by_pool)
   {
      _animations[name] = animation;
   }

   return animation;
}

//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::drawAnimations(sf::RenderTarget& target, const std::vector<std::string>& animations)
{
   if (animations.empty())
   {
      for (auto& [k, v] : _animations)
      {
         v->draw(target);
      }
   }
   else
   {
      for (const auto& key : animations)
      {
         const auto& animation = _animations.find(key);

         if (animation != _animations.end())
         {
            animation->second->draw(target);
         }
      }
   }
}

//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::drawAnimations(sf::RenderTarget& color, sf::RenderTarget& normal, const std::vector<std::string>& animations)
{
   if (animations.empty())
   {
      for (auto& [k, v] : _animations)
      {
         v->draw(color, normal);
      }
   }
   else
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

   if (_garbage_collector_enabled)
   {
      for (auto it = _animations.begin(); it != _animations.end();)
      {
         if (it->second->_paused && !it->second->_looped)
         {
            it = _animations.erase(it);
         }
         else
         {
            ++it;
         }
      }
   }
}

//----------------------------------------------------------------------------------------------------------------------
const std::map<std::string, std::shared_ptr<Animation>>& AnimationPool::getAnimations()
{
   return _animations;
}

//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::deserialize(const std::string& data)
{
   try
   {
      json config = json::parse(data);

      for (const auto& item : config.get<json::object_t>())
      {
         const auto name = item.first;
         const auto settings = std::make_shared<AnimationSettings>(item.second.get<AnimationSettings>());
         _settings[name] = settings;

         if (!settings->_valid)
         {
            Log::Error() << name << " frame durations vs. sprite count mismatch (" << settings->_frame_durations.size()
                         << " != " << settings->_sprite_count << ")";
         }

         auto texture = TexturePool::getInstance().get(settings->_texture_path);
         settings->_texture = texture;

         const auto normal_map_filename =
            (settings->_texture_path.stem().string() + "_normals" + settings->_texture_path.extension().string());
         const auto normal_map_path = (settings->_texture_path.parent_path() / normal_map_filename);

         if (std::filesystem::exists(normal_map_path))
         {
            auto normal_map = TexturePool::getInstance().get(normal_map_path);
            settings->_normal_map = normal_map;
         }
      }
   }
   catch (const std::exception& e)
   {
      Log::Error() << e.what();
   }
}

//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::deserializeFromFile(const std::string& filename)
{
   std::ifstream ifs(filename, std::ifstream::in);

   if (!ifs.good())
   {
      Log::Error() << "file " << filename << " not found";
   }

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

//----------------------------------------------------------------------------------------------------------------------
void AnimationPool::setGarbageCollectorEnabled(bool enabled)
{
   _garbage_collector_enabled = enabled;
}
