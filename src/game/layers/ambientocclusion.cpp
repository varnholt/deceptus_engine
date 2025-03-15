#include "ambientocclusion.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

#include "framework/tools/log.h"
#include "game/io/texturepool.h"
#include "game/player/player.h"

namespace
{
// increase the range if you have smaller AO block sizes
constexpr int32_t chunk_range_x_left = 4;
constexpr int32_t chunk_range_x_right = 4;
constexpr int32_t chunk_range_y_left = 3;
constexpr int32_t chunk_range_y_right = 3;
}  // namespace

void AmbientOcclusion::load(const std::filesystem::path& path, const std::string& base_filename)
{
   _config = Config(path, base_filename);
   _texture = TexturePool::getInstance().get(_config._texture_filename);

   if (_texture == nullptr || _texture->getSize().x == 0 || _texture->getSize().y == 0)
   {
      Log::Error() << "bad ambient occlusion texture";
      return;
   }

   auto x_index_px = 0;
   auto y_index_px = 0;
   auto quad_index = 0;
   auto x_px = 0;
   auto y_px = 0;
   auto width_px = 0;
   auto height_px = 0;

   auto group_x = 0;
   auto group_y = 0;

   std::string line;
   std::ifstream uv_file(_config._uv_filename);
   if (!uv_file.is_open())
   {
      return;
   }

   while (uv_file.good())
   {
      std::getline(uv_file, line);
      std::sscanf(line.c_str(), "%d;%d;%d;%d;%d", &quad_index, &x_px, &y_px, &width_px, &height_px);

      const auto x_index_px_prev = x_index_px;
      x_index_px = (quad_index * width_px) % _texture->getSize().x;
      if (x_index_px == 0 && x_index_px_prev != 0)
      {
         y_index_px += height_px;
      }

      sf::Sprite sprite(*_texture);
      sprite.setPosition({static_cast<float>(x_px - _config._offset_x_px), static_cast<float>(y_px - _config._offset_y_px)});
      sprite.setTextureRect({{x_index_px, y_index_px}, {width_px, height_px}});

      group_x = (x_px >> 8);
      group_y = (y_px >> 8);
      _sprite_map[group_y][group_x].push_back(sprite);
   }

   uv_file.close();
}

void AmbientOcclusion::draw(sf::RenderTarget& window)
{
   const auto& player_pos_px = Player::getCurrent()->getPixelPositionInt();

   const int32_t player_chunk_x = player_pos_px.x >> 8;
   const int32_t player_chunk_y = player_pos_px.y >> 8;

   for (auto y = player_chunk_y - chunk_range_y_left; y < player_chunk_y + chunk_range_y_right; y++)
   {
      const auto& y_it = _sprite_map.find(y);
      if (y_it == _sprite_map.end())
      {
         continue;
      }

      for (auto x = player_chunk_x - chunk_range_x_left; x < player_chunk_x + chunk_range_x_right; x++)
      {
         const auto& x_it = y_it->second.find(x);
         if (x_it == y_it->second.end())
         {
            continue;
         }

         // Log::Info() << "draw " << x_it->second.size() << " sprites";

         for (const auto& sprite : x_it->second)
         {
            window.draw(sprite, {sf::BlendAlpha});
         }
      }
   }
}

int32_t AmbientOcclusion::getZ() const
{
   return _config._z_index;
}

AmbientOcclusion::Config::Config(const std::filesystem::path& path, const std::string& base_filename)
    : _path(path), _base_filename(base_filename)
{
   // read config file
   const auto config_filename = path / "ambient_occlusion.json";
   std::ifstream ifs(config_filename, std::ifstream::in);
   auto c = static_cast<char>(ifs.get());
   std::string data;

   while (ifs.good())
   {
      data.push_back(c);
      c = static_cast<char>(ifs.get());
   }

   ifs.close();

   // parse json
   nlohmann::json j;
   try
   {
      j = nlohmann::json::parse(data);
   }
   catch (const std::exception& e)
   {
      std::cerr << e.what() << std::endl;
   }

   if (j.find("texture_filename") != j.end())
   {
      _texture_filename = (path / j.at("texture_filename").get<std::string>()).string();
   }
   else
   {
      _texture_filename = (path / _base_filename).string();
   }

   if (j.find("uv_filename") != j.end())
   {
      _uv_filename = (path / j.at("uv_filename").get<std::string>()).string();
   }
   else
   {
      _uv_filename = (path / (base_filename + "_ao_tiles.uv")).string();
   }

   if (j.find("z_index") != j.end())
   {
      _z_index = j.at("z_index").get<int32_t>();
   }
   else
   {
      _z_index = static_cast<int32_t>(ZDepth::Player);
   }

   if (j.find("offset_x_px") != j.end())
   {
      _offset_x_px = j.at("offset_x_px").get<int32_t>();
   }

   if (j.find("offset_y_px") != j.end())
   {
      _offset_y_px = j.at("offset_y_px").get<int32_t>();
   }

   if (!std::filesystem::exists(_texture_filename))
   {
      Log::Error() << "ambient occlusion texture not found (" << _texture_filename << ")";
   }

   if (!std::filesystem::exists(_uv_filename))
   {
      Log::Error() << "ambient occlusion UV file not found (" << _uv_filename << ")";
   }
}
