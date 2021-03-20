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

   DrawMode getDrawMode() const;
   void setDrawMode(const DrawMode& draw_mode);


protected:

   virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;


private:

   struct AnimatedTileFrame
   {
      int _x = 0;
      int _y = 0;
      int _duration = 0;
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
   std::shared_ptr<sf::Texture> _active_texture;

   std::vector<AnimatedTile*> _animations;

   int _z = 0;
   bool _visible = true;
   DrawMode _draw_mode = DrawMode::ColorMap;
};

