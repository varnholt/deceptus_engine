#pragma once

#include "game/constants.h"
#include "game/sfmlanimatedsprite.h"

#include <array>
#include <list>
#include <memory>

#include "json/json.hpp"
using json = nlohmann::json;


class PlayerAnimation
{
   public:

      struct PlayerAnimationSetup
      {
         int mWidth = 0;
         int mHeight = 0;
         int mSprites = 0;
         float mOriginX = 0.0f;
         float mOriginY = 0.0f;
         float mFrameTime = 0.0f;
         int mAnimationDuration = 0;
         sf::Texture mTexture;
         std::vector<sf::IntRect> mFrames;
      };

      void initialize();
      void add(AnimationType type, float x, float y);
      void updateAnimations(float dt);
      const std::vector<SpriteAnimation*>& getAnimations();

      static PlayerAnimation& getInstance();

   private:

      PlayerAnimation() = default;

      bool sInitialized = false;
      std::map<AnimationType, std::shared_ptr<PlayerAnimationSetup>> mSetups;
      std::vector<SpriteAnimation*> mAnimations;

      void deserialize(const std::string& data);
      void deserializeFromFile(const std::string& filename = "data/config/levels.json");


      static PlayerAnimation sPlayerAnimation;

};

void from_json(const json& j, PlayerAnimation& item);

