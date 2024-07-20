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
   nlohmann::json json_config;
   try
   {
      json_config = nlohmann::json::parse(data);
   }
   catch (const std::exception& e)
   {
      std::cerr << e.what() << std::endl;
   }

   // read config from json
   _config = json_config.get<Config>();
   const auto texture = (path / _config._texture_filename).string();
   const auto uv = (path / _config._uv_filename).string();

   if (!std::filesystem::exists(texture))
   {
      Log::Error() << "need to create an ambient occlusion map (" << texture << ")";
      return;
   }

   _texture = TexturePool::getInstance().get(texture);

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
   std::ifstream uv_file(uv);
   if (uv_file.is_open())
   {
      while (uv_file.good())
      {
         std::getline(uv_file, line);
         std::sscanf(line.c_str(), "%d;%d;%d;%d;%d", &i, &x, &y, &w, &h);

         sf::Sprite sprite;
         sprite.setPosition(static_cast<float>(x - 5), static_cast<float>(y - 6));
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
   else
   {
      Log::Error() << "AmbientOcclusion::load: unable to open uv file: " << uv;
   }
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

void from_json(const nlohmann::json& j, AmbientOcclusion::Config& settings)
{
   settings._texture_filename = j.at("texture_filename").get<std::string>();
   settings._uv_filename = j.at("uv_filename").get<std::string>();
   settings._z_index = j.at("z_index").get<int32_t>();
}
