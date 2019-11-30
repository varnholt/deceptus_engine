#pragma once

#include <filesystem>
#include <functional>
#include <stdint.h>

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

#include "gamemechanism.h"

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;


class Lever : public GameMechanism
{

public:

   using Callback = std::function<void(int32_t)>;

   enum class Type {
      TwoState,
      TriState
   };

   enum class State {
      Left   = -1,
      Middle = 0,
      Right  = 1,
   };

   Lever() = default;

   void update(const sf::Time& dt) override;
   void draw(sf::RenderTarget& target) override;

   void toggle();
   void setCallbacks(const std::vector<Callback>& callbacks);

   static void addSearchRect(TmxObject* rect);
   static void merge(
      const std::vector<std::shared_ptr<GameMechanism>>& lasers,
      const std::vector<std::shared_ptr<GameMechanism>>& platforms,
      const std::vector<std::shared_ptr<GameMechanism>>& fans,
      const std::vector<std::shared_ptr<GameMechanism>>& belts
   );

   static std::vector<std::shared_ptr<GameMechanism>> load(
      TmxLayer* layer,
      TmxTileSet* tileSet,
      const std::filesystem::path& basePath,
      const std::shared_ptr<b2World>&
   );

   bool getPlayerAtLever() const;
   void setPlayerAtLever(bool playerAtLever);


private:

   Type mType = Type::TwoState;

   State mTargetState = State::Left;
   State mPreviousState = State::Left;

   std::vector<Callback> mCallbacks;
   sf::Rect<int32_t> mRect;
   bool mPlayerAtLever = false;
   sf::Sprite mSprite;
   int32_t mOffset = 0;
   bool mLeftAligned = true;
   int32_t mDir = 0;

   static sf::Texture sTexture;

   static std::vector<TmxObject*> mRectangles;
   static std::vector<std::shared_ptr<Lever>> sLevers;
   void updateSprite();
};


