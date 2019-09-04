#ifndef MOVINGPLATFORM_H
#define MOVINGPLATFORM_H

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


class MovingPlatform : public GameNode
{

protected:
   sf::Texture mTexture;
   std::vector<sf::Sprite> mSprites;
   b2Body* mBody = nullptr;
   sf::Vector2i mTilePosition;
   float mX = 0.0f;
   float mY = 0.0f;
   int mWidth = 0;
   int mHeight = 1;
   int mZ = 0;
   float mTime = 0.0f;
   PathInterpolation mInterpolation;

public:
   MovingPlatform(GameNode* parent);

   static std::vector<std::shared_ptr<MovingPlatform>> load(
      TmxLayer* layer,
      TmxTileSet* tileSet,
      const std::filesystem::path &basePath,
      const std::shared_ptr<b2World>& world
   );

   static void link(const std::vector<std::shared_ptr<MovingPlatform>>& platforms, TmxObject* tmxObject);

   void draw(sf::RenderTarget& target);
   void update(const sf::Time& dt);
   void setupBody(const std::shared_ptr<b2World>& world);
   void addSprite(const sf::Sprite&);
   void setOffset(float x, float y);
   int getZ() const;
   void setZ(int z);


protected:
   double CosineInterpolate(
      double y1,double y2,
      double mu
   );

private:
   void updateTransform();
};

#endif // MOVINGPLATFORM_H
