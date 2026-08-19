#include "ambientocclusion.h"

#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

#include "framework/tools/log.h"
#include "game/io/texturepool.h"
#ifdef DEVELOPMENT_MODE
#include "game/debug/drawcallcounter.h"
#endif
#include "game/player/playerregistry.h"

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
   Log::Info() << "loading ao: " << path.string() << "/" << base_filename;

   _config = Config(path, base_filename);

   if (_config._valid)
   {
      _texture = TexturePool::getInstance().get(_config._texture_filename);
   }

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

      const auto left_px = static_cast<float>(x_px - _config._offset_x_px);
      const auto top_px = static_cast<float>(y_px - _config._offset_y_px);
      const auto right_px = left_px + static_cast<float>(width_px);
      const auto bottom_px = top_px + static_cast<float>(height_px);

      const auto texture_left_px = static_cast<float>(x_index_px);
      const auto texture_top_px = static_cast<float>(y_index_px);
      const auto texture_right_px = texture_left_px + static_cast<float>(width_px);
      const auto texture_bottom_px = texture_top_px + static_cast<float>(height_px);

      // clang-format off
      std::array<sf::Vertex, 4> quad;
      quad[0].position = sf::Vector2f(left_px,  top_px);
      quad[1].position = sf::Vector2f(right_px, top_px);
      quad[2].position = sf::Vector2f(right_px, bottom_px);
      quad[3].position = sf::Vector2f(left_px,  bottom_px);

      quad[0].texCoords = sf::Vector2f(texture_left_px,  texture_top_px);
      quad[1].texCoords = sf::Vector2f(texture_right_px, texture_top_px);
      quad[2].texCoords = sf::Vector2f(texture_right_px, texture_bottom_px);
      quad[3].texCoords = sf::Vector2f(texture_left_px,  texture_bottom_px);

      quad[0].color = sf::Color::White;
      quad[1].color = sf::Color::White;
      quad[2].color = sf::Color::White;
      quad[3].color = sf::Color::White;
      // clang-format on

      group_x = (x_px >> 8);
      group_y = (y_px >> 8);

      auto& chunk_vertices = _vertex_map[group_y][group_x];
      chunk_vertices.push_back(quad[0]);
      chunk_vertices.push_back(quad[1]);
      chunk_vertices.push_back(quad[2]);

      chunk_vertices.push_back(quad[0]);
      chunk_vertices.push_back(quad[2]);
      chunk_vertices.push_back(quad[3]);
   }

   uv_file.close();
}

void AmbientOcclusion::draw(sf::RenderTarget& window, const sf::RenderStates& states)
{
   const auto& player_pos_px = PlayerRegistry::getFirst()->getPixelPositionInt();

   const int32_t player_chunk_x = player_pos_px.x >> 8;
   const int32_t player_chunk_y = player_pos_px.y >> 8;

   sf::RenderStates draw_states = states;
   draw_states.texture = _texture.get();
   draw_states.blendMode = sf::BlendAlpha;

   // every quad carries the same texture, blend mode and transform, so the visible chunks
   // concatenate into a single call. drawing them one sprite at a time cost a draw call per quad -
   // a median of 230 per frame in the catacombs, and up to 444
   _batched_vertices.clear();

   for (auto y = player_chunk_y - chunk_range_y_left; y < player_chunk_y + chunk_range_y_right; y++)
   {
      const auto& y_it = _vertex_map.find(y);
      if (y_it == _vertex_map.end())
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

         const auto& chunk_vertices = x_it->second;
         _batched_vertices.insert(_batched_vertices.end(), chunk_vertices.begin(), chunk_vertices.end());
      }
   }

   if (_batched_vertices.empty())
   {
      return;
   }

#ifdef DECEPTUS_VRSFML
   window.draw(std::span<const sf::Vertex>{_batched_vertices.data(), _batched_vertices.size()}, sf::PrimitiveType::Triangles, draw_states);
#else
   window.draw(_batched_vertices.data(), _batched_vertices.size(), sf::PrimitiveType::Triangles, draw_states);
#endif

#ifdef DEVELOPMENT_MODE
   DrawCallCounter::ambient_occlusion_draw_calls++;
#endif
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

   if (data.empty())
   {
      return;
   }

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

   const auto texture_filename_valid = std::filesystem::exists(_texture_filename);
   const auto uv_filename_valid = std::filesystem::exists(_uv_filename);

   if (!texture_filename_valid)
   {
      Log::Error() << "ambient occlusion texture not found (" << _texture_filename << ")";
   }

   if (!uv_filename_valid)
   {
      Log::Error() << "ambient occlusion UV file not found (" << _uv_filename << ")";
   }

   _valid = texture_filename_valid && uv_filename_valid;
}
