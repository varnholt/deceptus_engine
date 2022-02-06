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


/*! \brief A tile map implementation that loads its data from TMX structures.
 *         A tile map is an array of tiles.
 *
 *  For each tile loaded from a TMX tile layer, an OpenGL quad is created using the UV and texture
 *  from the TMX tile set that's provided. TileMap also supports tile animations, implemented in AnimatedTile.
 */
class TileMap : public sf::Drawable, public sf::Transformable
{
public:

   TileMap() = default;
   ~TileMap() override;

   virtual bool load(TmxLayer* layer, TmxTileSet* tileSet, const std::filesystem::path& basePath);
   virtual void update(const sf::Time& dt);
   virtual void draw(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states) const;

   int getZ() const;
   void setZ(int getZ);

   bool isVisible() const;
   void setVisible(bool visible);

   void hideTile(int x, int y);

   const std::string& getLayerName() const;


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

   int _z_index = 0;
   bool _visible = true;
   std::string _layer_name;
   std::string _tileset_name;
};

