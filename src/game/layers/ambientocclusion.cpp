#include "ambientocclusion.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

#include "framework/math/sfmlmath.h"
#include "framework/tools/log.h"
#include "game/io/texturepool.h"
#include "player/player.h"

void AmbientOcclusion::load(const std::filesystem::path& path, const std::string& base_filename)
{
   _config = Config(path, base_filename);
   _texture = TexturePool::getInstance().get(_config._texture_filename);

   auto xi = 0;
   auto yi = 0;
   auto i = 0;
   auto x = 0;
   auto y = 0;
   auto w = 0;
   auto h = 0;

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
      std::sscanf(line.c_str(), "%d;%d;%d;%d;%d", &i, &x, &y, &w, &h);

      sf::Sprite sprite;
      sprite.setPosition(static_cast<float>(x - _config._offset_x_px), static_cast<float>(y - _config._offset_y_px));
      sprite.setTexture(*_texture);
      sprite.setTextureRect({xi, yi, w, h});

      group_x = (x >> 8);
      group_y = (y >> 8);
      _sprite_map[group_y][group_x].push_back(sprite);

      xi += w;
      if (xi == static_cast<int32_t>(_texture->getSize().x))
      {
         xi = 0;
         yi += h;
      }
   }

   uv_file.close();
}

void AmbientOcclusion::draw(sf::RenderTarget& window)
{
   const auto& player_pos_px = Player::getCurrent()->getPixelPositionInt();

   const int32_t player_chunk_x = player_pos_px.x >> 8;
   const int32_t player_chunk_y = player_pos_px.y >> 8;

   // increase the range if you have smaller AO block sizes
   constexpr int32_t rxl = 4;
   constexpr int32_t rxr = 4;

   constexpr int32_t ryl = 3;
   constexpr int32_t ryr = 3;

   for (auto y = player_chunk_y - ryl; y < player_chunk_y + ryr; y++)
   {
      const auto& y_it = _sprite_map.find(y);
      if (y_it == _sprite_map.end())
      {
         continue;
      }

      for (auto x = player_chunk_x - rxl; x < player_chunk_x + rxr; x++)
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
