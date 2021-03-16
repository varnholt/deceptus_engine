#pragma once

// sfml
#include "SFML/Graphics.hpp"

// std
#include <array>
#include <filesystem>
#include <vector>


struct TmxAnimation;
struct TmxLayer;
struct TmxTile;
struct TmxTileSet;


class TileMap : public sf::Drawable, public sf::Transformable
{
public:

   TileMap() = default;

   bool load(
      TmxLayer* layer,
      TmxTileSet* tileSet,
      const std::filesystem::path& basePath
   );


   void update(const sf::Time& dt);

   int getZ() const;
   void setZ(int getZ);

   void hideTile(int x, int y);

   bool isVisible() const;
   void setVisible(bool visible);


protected:

   virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;


private:

   struct AnimatedTileFrame
   {
      int mX = 0;
      int mY = 0;
      int mDuration = 0;
   };

   struct AnimatedTile
   {
      virtual ~AnimatedTile();

      int mTileX = 0;
      int mTileY = 0;
      std::vector<AnimatedTileFrame*> mFrames;
      int mCurrentFrame = 0;
      float mElapsed = 0.0f;
      float mDuration = 0.0f;
      sf::Vertex mVertices[4];
      bool mVisible = true;
      TmxAnimation* mAnimation = nullptr;
   };

   sf::Vector2u mTileSize;

   mutable std::map<int32_t, std::map<int32_t, sf::VertexArray>> mVerticesStaticBlocks;
   sf::VertexArray mVerticesAnimated;

   std::shared_ptr<sf::Texture> mTexture;
   std::shared_ptr<sf::Texture> mBumpMap;

   std::vector<AnimatedTile*> mAnimations;

   int mZ = 0;
   bool mVisible = true;
};

