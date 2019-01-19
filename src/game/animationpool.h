#pragma once

#include "game/animation.h"
#include "game/animationsettings.h"
#include "game/constants.h"

#include <array>
#include <list>
#include <memory>

#include "json/json.hpp"


class AnimationPool
{
   public:

      void initialize();
      void add(AnimationType type, float x, float y);
      void updateAnimations(float dt);
      const std::vector<Animation*>& getAnimations();

      static AnimationPool& getInstance();

   private:

      AnimationPool() = default;

      bool sInitialized = false;
      std::map<AnimationType, std::shared_ptr<AnimationSettings>> mSetups;
      std::vector<Animation*> mAnimations;

      void deserialize(const std::string& data);
      void deserializeFromFile(const std::string& filename = "data/config/levels.json");


      static AnimationPool sPlayerAnimation;

};

void from_json(const nlohmann::json& j, AnimationPool& item);

