#pragma once

#include "game/animation.h"
#include "game/animationsettings.h"
#include "game/constants.h"

#include <array>
#include <list>
#include <memory>

class AnimationPool
{
   public:

      void initialize();

      std::shared_ptr<Animation> add(
         const std::string& animationName,
         float x = 0.0f,
         float y = 0.0f,
         bool autoPlay = true,
         bool managedByPool = true
      );

      void drawAnimations(sf::RenderTarget& target, const std::vector<std::string>& animations);
      void updateAnimations(const sf::Time& dt);
      const std::map<std::string, std::shared_ptr<Animation> >& getAnimations();

      static AnimationPool& getInstance();

   private:

      AnimationPool() = default;

      void deserialize(const std::string& data);
      void deserializeFromFile(const std::string& filename = "data/sprites/animations.json");

      bool _initialized = false;

      std::map<std::string, std::shared_ptr<AnimationSettings>> _settings;
      std::map<std::string, std::shared_ptr<sf::Texture>> _textures;
      std::map<std::string, std::shared_ptr<Animation>> _animations;

      static AnimationPool _player_animation;

};
