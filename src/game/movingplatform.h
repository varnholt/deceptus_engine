#pragma once

#include "gamemechanism.h"
#include "gamenode.h"

#include "pathinterpolation.h"
#include <filesystem>
#include "Box2D/Box2D.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>


struct TmxLayer;
struct TmxObject;
struct TmxTileSet;


class MovingPlatform : public GameMechanism, public GameNode
{

public:
   MovingPlatform(GameNode* parent);

   static std::vector<std::shared_ptr<MovingPlatform>> load(
      TmxLayer* layer,
      TmxTileSet* tileSet,
      const std::filesystem::path &basePath,
      const std::shared_ptr<b2World>& world
   );

   static void link(const std::vector<std::shared_ptr<MovingPlatform>>& platforms, TmxObject* tmxObject);

   void draw(sf::RenderTarget& target) override;
   void update(const sf::Time& dt) override;

   void setupBody(const std::shared_ptr<b2World>& world);
   void addSprite(const sf::Sprite&);
   void setOffset(float x, float y);
   b2Body* getBody();


private:

   void setupTransform();

   double CosineInterpolate(
      double y1,double y2,
      double mu
   );


private:
   sf::Texture mTexture;
   std::vector<sf::Sprite> mSprites;
   b2Body* mBody = nullptr;
   sf::Vector2i mTilePosition;
   float mX = 0.0f;
   float mY = 0.0f;
   int mWidth = 0;
   int mHeight = 1;
   float mTime = 0.0f;
   PathInterpolation mInterpolation;
};

