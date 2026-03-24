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

/// \brief drawable TMX tile layer with optional animated tiles and normal-map rendering.
class TileMap : public sf::Drawable, public sf::Transformable
{
public:
   TileMap() = default;

   /// \brief releases dynamic animation frame storage.
   ~TileMap() override;

   /// \brief loads tile geometry, textures, and animation metadata from TMX layer data.
   /// \param layer source TMX layer with tile indices and properties.
   /// \param tileSet TMX tileset referenced by \p layer.
   /// \param basePath base path used to resolve tileset textures.
   /// \return true when loading succeeded.
   virtual bool
   load(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TmxTileSet>& tileSet, const std::filesystem::path& basePath);

   /// \brief advances visible animated tiles and rebuilds animated vertex data.
   /// \param dt elapsed frame time.
   virtual void update(const sf::Time& dt);

   /// \brief draws color map and optional normal map to separate render targets.
   /// \param color color render target.
   /// \param normal normal render target.
   /// \param states render states forwarded to draw calls.
   virtual void draw(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states) const;

   /// \brief draws this layer to a single render target.
   /// \param target render target.
   /// \param states render states forwarded to draw calls.
   void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

   /// \brief renders the layer to an image file for debugging.
   /// \param output_path target png file path.
   /// \return true when the image was written successfully.
   bool dumpToPng(const std::filesystem::path& output_path) const;

   /// \brief returns z-order used for layer sorting.
   /// \return layer z index.
   int32_t getZ() const;

   /// \brief sets z-order used for layer sorting.
   /// \param z layer z index.
   void setZ(int32_t z);

   /// \brief reports whether the layer is currently visible.
   /// \return true when rendering is enabled.
   bool isVisible() const;

   /// \brief toggles layer visibility.
   /// \param visible true to draw the layer.
   void setVisible(bool visible);

   /// \brief hides one tile at tile-grid coordinates.
   /// \param x x coordinate in pixels.
   /// \param y y coordinate in pixels.
   void hideTile(int32_t x, int32_t y);

   /// \brief returns source TMX layer name.
   /// \return reference to layer name.
   const std::string& getLayerName() const;

protected:
   /// \brief draws static and animated vertex buffers near the player.
   /// \param target render target.
   /// \param states render states forwarded to draw calls.
   void drawVertices(sf::RenderTarget& target, sf::RenderStates states) const;

private:
   /// \brief stores one tile quad as an animated tile entry.
   /// \param quad tile quad geometry and initial uv data.
   /// \param tx tile x index in tile coordinates.
   /// \param ty tile y index in tile coordinates.
   /// \param animation TMX animation description for this tile.
   void storeAnimation(const std::array<sf::Vertex, 4>& quad, int32_t tx, int32_t ty, const std::shared_ptr<TmxAnimation>& animation);

   /// \brief stores one tile quad inside the static block vertex cache.
   /// \param quad tile quad geometry and uv data.
   /// \param tx tile x index in tile coordinates.
   /// \param ty tile y index in tile coordinates.
   /// \param parallax_scale layer parallax factor used for block lookup.
   void storeStaticVertices(const std::array<sf::Vertex, 4>& quad, const int32_t tx, const int32_t ty, float parallax_scale);

   /// \brief one animation frame entry inside an animated tile.
   struct AnimatedTileFrame
   {
      int32_t _x_px = 0;
      int32_t _y_px = 0;
      int32_t _duration_ms = 0;
   };

   /// \brief runtime state for one animated tile instance.
   struct AnimatedTile
   {
      /// \brief releases owned animation frame pointers.
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
