#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>
#include "json/json.hpp"

class AmbientOcclusion
{
public:
   AmbientOcclusion() = default;

   void load(const std::filesystem::path& path, const std::string& ao_base_filename);
   void draw(sf::RenderTarget& window);
   int32_t getZ() const;

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
   Config _config;
   std::shared_ptr<sf::Texture> _texture;
   std::map<int32_t, std::map<int32_t, std::vector<sf::Sprite>>> _sprite_map;
};
