#ifndef TILEMAP_H
#define TILEMAP_H

// sfml
#include "SFML/Graphics.hpp"

// std
#include <filesystem>
#include <vector>


struct TmxAnimation;
struct TmxLayer;
struct TmxTile;
struct TmxTileSet;


class TileMap : public sf::Drawable, public sf::Transformable
{

private:

   struct AnimatedTileFrame
   {
      int mX = 0;
      int mY = 0;
      int mDuration = 0;
   };

   struct AnimatedTile
   {
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

   sf::VertexArray mVerticesStatic;
   sf::VertexArray mVerticesAnimated;

   sf::Texture mTexture;

   std::vector<AnimatedTile*> mAnimations;

   int mZ = 0;
   bool mVisible = true;


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

   void hideTile(int x, int y, int vertexOffset);

   bool isVisible() const;
   void setVisible(bool visible);


protected:

   virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
};

#endif // TILEMAP_H
