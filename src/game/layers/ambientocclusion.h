#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>
#include "json/json.hpp"
#include "framework/tools/asynctexturepool.h"

class AmbientOcclusion
{
public:
   AmbientOcclusion() = default;

   void load(const std::filesystem::path& path, const std::string& ao_base_filename);
   void loadAsync(const std::filesystem::path& path, const std::string& ao_base_filename);
   void update();  // Process any completed async loads
   void draw(sf::RenderTarget& window);
   int32_t getZ() const;
   bool isReady() const;  // Check if the texture is ready to render

   struct Config
   {
      Config() = default;
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
   struct UVData {
      int quad_index;
      int x_px, y_px;
      int width_px, height_px;
   };

   Config _config;
   std::shared_ptr<sf::Texture> _texture;
   AsyncTextureHandle _async_texture_handle;  // Handle for async loading
   bool _async_load_started = false;
   bool _texture_loaded = false;
   std::vector<UVData> _uv_data;  // Store UV data until texture is loaded
   std::map<int32_t, std::map<int32_t, std::vector<sf::Sprite>>> _sprite_map;
};
