#pragma once

// sfml
#include "SFML/Graphics.hpp"

// std
#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
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

   virtual bool
   load(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TmxTileSet>& tileSet, const std::filesystem::path& basePath);
   virtual void update(const sf::Time& dt);
   virtual void draw(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states) const;
   void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
   bool dumpToPng(const std::filesystem::path& output_path) const;

   int32_t getZ() const;
   void setZ(int32_t getZ);

   bool isVisible() const;
   void setVisible(bool visible);

   void hideTile(int32_t x, int32_t y);

   const std::string& getLayerName() const;

protected:
   void drawVertices(sf::RenderTarget& target, sf::RenderStates states) const;

private:
   void storeAnimation(const std::array<sf::Vertex, 4>& quad, int32_t tx, int32_t ty, const std::shared_ptr<TmxAnimation>& animation);
   void storeStaticVertices(const std::array<sf::Vertex, 4>& quad, const int32_t tx, const int32_t ty, float parallax_scale);

   struct AnimatedTileFrame
   {
      int32_t _x_px = 0;
      int32_t _y_px = 0;
      int32_t _duration_ms = 0;
   };

   struct AnimatedTile
   {
      virtual ~AnimatedTile();

      int32_t _tile_x = 0;
      int32_t _tile_y = 0;
      std::vector<AnimatedTileFrame*> _frames;
      int32_t _current_frame = 0;
      float _elapsed_ms = 0.0f;
      float _duration = 0.0f;
      sf::Vertex _vertices[4];
      bool _visible = true;
      std::shared_ptr<TmxAnimation> _animation;
   };

   sf::Vector2u _tile_size;

   mutable std::map<int32_t, std::map<int32_t, sf::VertexArray>> _vertices_static_blocks;
   sf::VertexArray _vertices_animated;

   std::shared_ptr<sf::Texture> _texture_map;
   std::shared_ptr<sf::Texture> _normal_map;

   std::vector<AnimatedTile*> _animations;

   int32_t _z_index = 0;
   bool _visible = true;
   std::string _layer_name;
   std::string _tileset_name;
   std::optional<sf::BlendMode> _blend_mode;
};
