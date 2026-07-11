#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>
#include "json/json.hpp"

/// \brief loads and renders precomputed ambient occlusion sprites around the player.
class AmbientOcclusion
{
public:
   /// \brief creates an empty ambient occlusion renderer.
   AmbientOcclusion() = default;

   /// \brief loads ao configuration, texture, and per-tile uv sprite placement data.
   /// \param path directory containing ambient_occlusion.json and referenced assets.
   /// \param ao_base_filename fallback texture base name used when json fields are missing.
   void load(const std::filesystem::path& path, const std::string& ao_base_filename);

   /// \brief draws only ao sprite chunks near the player's current chunk.
   /// \param window SFML render target that receives ambient occlusion sprites.
   /// \param states render states to apply (carries .view for WASM camera transform).
   void draw(sf::RenderTarget& window, const sf::RenderStates& states = sf::RenderStates{});

   /// \brief gets the render depth configured for this ao layer.
   /// \return z index used to sort the layer in the scene renderer.
   int32_t getZ() const;

   /// \brief parsed ambient occlusion file configuration and validation state.
   struct Config
   {
      /// \brief creates an empty, invalid configuration.
      Config() = default;

      /// \brief loads ambient occlusion settings from json and resolves file paths.
      /// \param path directory containing ambient_occlusion.json.
      /// \param base_filename fallback base filename for generated defaults.
      Config(const std::filesystem::path& path, const std::string& base_filename);

      std::filesystem::path _path;
      std::string _base_filename;

      std::string _texture_filename;
      std::string _uv_filename;
      int32_t _z_index{};
      int32_t _offset_x_px{};
      int32_t _offset_y_px{};
      bool _valid{false};
   };

private:
   Config _config;
   std::shared_ptr<sf::Texture> _texture;
   std::map<int32_t, std::map<int32_t, std::vector<sf::Sprite>>> _sprite_map;
};
