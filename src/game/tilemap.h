#pragma once

// sfml
#include "SFML/Graphics.hpp"

// std
#include <array>
#include <filesystem>
#include <vector>

#include "constants.h"

struct TmxAnimation;
struct TmxLayer;
struct TmxTile;
struct TmxTileSet;


class TileMap : public sf::Drawable, public sf::Transformable
{
public:

   TileMap() = default;
   ~TileMap() override;

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

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states) const;


protected:

   void draw(sf::RenderTarget& target, sf::RenderStates states) const override;


private:

   void drawVertices(sf::RenderTarget &target, sf::RenderStates states) const;

   struct AnimatedTileFrame
   {
      int _x_px = 0;
      int _y_px = 0;
      int _duration_ms = 0;
   };

   struct AnimatedTile
   {
      virtual ~AnimatedTile();

      int _tile_x = 0;
      int _tile_y = 0;
      std::vector<AnimatedTileFrame*> _frames;
      int _current_frame = 0;
      float _elapsed_ms = 0.0f;
      float _duration = 0.0f;
      sf::Vertex _vertices[4];
      bool _visible = true;
      TmxAnimation* _animation = nullptr;
   };

   sf::Vector2u _tile_size;

   mutable std::map<int32_t, std::map<int32_t, sf::VertexArray>> _vertices_static_blocks;
   sf::VertexArray _vertices_animated;

   std::shared_ptr<sf::Texture> _texture_map;
   std::shared_ptr<sf::Texture> _normal_map;

   std::vector<AnimatedTile*> _animations;

   int _z = 0;
   bool _visible = true;
};

