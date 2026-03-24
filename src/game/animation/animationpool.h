#pragma once

#include "game/animation/animation.h"
#include "game/animation/animationsettings.h"
#include "game/constants.h"

#include <array>
#include <list>
#include <memory>

/// \brief manager that loads animation settings from json and creates or updates animation instances.
class AnimationPool
{
public:
   /// \brief constructs a pool bound to one animation settings json file.
   /// \param file_path path to the json file that defines animation settings.
   AnimationPool(const std::string& file_path);

   /// \brief lazily loads animation settings from disk once.
   void initialize();
   /// \brief resets initialization state and loads settings from disk again.
   void reload();

   /// \brief creates an animation instance from stored settings and optionally registers it in the pool.
   /// \param name animation id as defined in the settings map.
   /// \param x initial x position in world pixels.
   /// \param y initial y position in world pixels.
   /// \param auto_play true to start playback immediately, false to create paused.
   /// \param managed_by_pool true to store the created animation in the pool map by name.
   /// \return shared pointer to the created animation instance.
   std::shared_ptr<Animation>
   create(const std::string& name, float x = 0.0f, float y = 0.0f, bool auto_play = true, bool managed_by_pool = true);

   /// \brief draws managed animations to one render target, optionally filtered by animation names.
   /// \param target render target that receives animation geometry.
   /// \param animations optional name filter; when empty, all managed animations are drawn.
   void drawAnimations(sf::RenderTarget& target, const std::vector<std::string>& animations = {});
   /// \brief draws managed animations to color and normal targets, optionally filtered by names.
   /// \param target render target for color output.
   /// \param normal render target for normal-map output.
   /// \param animations optional name filter; when empty, all managed animations are drawn.
   void drawAnimations(sf::RenderTarget& target, sf::RenderTarget& normal, const std::vector<std::string>& animations = {});

   /// \brief updates all managed animations and removes finished non-looping ones when garbage collection is enabled.
   /// \param dt elapsed frame time since the previous update.
   void updateAnimations(const sf::Time& dt);
   /// \brief exposes the current map of managed runtime animation instances.
   /// \return reference to the managed animation map keyed by animation name.
   const std::map<std::string, std::shared_ptr<Animation>>& getAnimations();

   /// \brief enables or disables automatic removal of paused non-looping managed animations.
   /// \param newGarbage_collector_enabled true to erase finished one-shot animations during update.
   void setGarbageCollectorEnabled(bool newGarbage_collector_enabled);

   /// \brief exposes loaded animation setting definitions.
   /// \return reference to the settings map keyed by animation name.
   const std::map<std::string, std::shared_ptr<AnimationSettings>>& settings() const;

   /// \brief serializes current settings back to the json file path owned by this pool.
   void saveToJson();
   /// \brief clears current settings and animations, then reloads settings from json.
   void reloadFromJson();

   enum class UpdateFlag
   {
      Settings,
      Texture,
      NormalMap,
      All
   };

   /// \brief rebuilds runtime animations from settings and refreshes selected resource categories.
   /// \param flag selects whether to refresh settings only, textures, normal maps, or everything.
   void recreateAnimationsFromSettings(UpdateFlag flag = UpdateFlag::Settings);

private:
   /// \brief parses animation settings json text, populates settings, and loads referenced textures.
   /// \param data json text containing animation definitions.
   void deserialize(const std::string& data);
   /// \brief reads animation settings from a file and forwards content to json deserialization.
   /// \param filename path to the settings file to read.
   void deserializeFromFile(const std::string& filename);

   bool _initialized = false;

   std::map<std::string, std::shared_ptr<AnimationSettings>> _settings;
   std::map<std::string, std::shared_ptr<Animation>> _animations;
   std::string _file_path;
   bool _garbage_collector_enabled{true};
};
