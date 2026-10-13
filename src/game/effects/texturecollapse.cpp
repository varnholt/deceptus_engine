#include "game/effects/texturecollapse.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "framework/tools/sfmlcompat.h"

namespace
{
constexpr auto collapse_piece_size_px = 24;
// slow enough to watch the ground break apart piece by piece while the player drops through it
constexpr auto collapse_gravity_px_s2 = 600.0f;
constexpr auto collapse_max_delay_s = 0.6f;
constexpr auto collapse_fade_start_s = 0.9f;
constexpr auto collapse_fade_duration_s = 0.6f;
}  // namespace

void TextureCollapse::start(
   const std::shared_ptr<sf::Texture>& texture_map,
   const std::shared_ptr<sf::Texture>& normal_map,
   const sf::Vector2f& top_left_px
)
{
   if (!texture_map)
   {
      return;
   }

   _texture_map = texture_map;
   _normal_map = normal_map;

   _collapse_sprite = sfcompat::createSprite(*_texture_map);
   sfcompat::setOrigin(*_collapse_sprite, {collapse_piece_size_px * 0.5f, collapse_piece_size_px * 0.5f});

   _collapse_elapsed_s = 0.0f;
   _collapse_pieces.clear();

   // the pieces cover the texture, which may reach above the rectangle
   const auto texture_size = _texture_map->getSize();
   const auto columns = static_cast<int32_t>(texture_size.x) / collapse_piece_size_px;
   const auto rows = static_cast<int32_t>(texture_size.y) / collapse_piece_size_px;

   for (auto row = 0; row < rows; ++row)
   {
      for (auto column = 0; column < columns; ++column)
      {
         CollapsePiece piece;
         piece._texture_rect = {
            {column * collapse_piece_size_px, row * collapse_piece_size_px}, {collapse_piece_size_px, collapse_piece_size_px}
         };
         piece._position_px = {
            top_left_px.x + (column + 0.5f) * collapse_piece_size_px, top_left_px.y + (row + 0.5f) * collapse_piece_size_px
         };

         // the top row breaks first and the middle of the hole goes before its edges
         const auto random_unit = (std::rand() % 256) / 255.0f;
         const auto distance_from_centre = std::abs(column - (columns - 1) * 0.5f) / std::max(1.0f, columns * 0.5f);
         piece._delay_s = collapse_max_delay_s * (0.5f * distance_from_centre + 0.3f * random_unit + 0.2f * row / std::max(1, rows));
         piece._rotation_speed_degrees_s = (random_unit - 0.5f) * 240.0f;
         _collapse_pieces.push_back(piece);
      }
   }
}

void TextureCollapse::update(const sf::Time& dt)
{
   if (_collapse_pieces.empty())
   {
      return;
   }

   const auto dt_s = dt.asSeconds();
   _collapse_elapsed_s += dt_s;

   for (auto& piece : _collapse_pieces)
   {
      if (_collapse_elapsed_s < piece._delay_s)
      {
         continue;
      }

      piece._velocity_y_px_s += collapse_gravity_px_s2 * dt_s;
      piece._position_px.y += piece._velocity_y_px_s * dt_s;
      piece._rotation_degrees += piece._rotation_speed_degrees_s * dt_s;
   }

   if (_collapse_elapsed_s > collapse_max_delay_s + collapse_fade_start_s + collapse_fade_duration_s)
   {
      _collapse_pieces.clear();
   }
}

void TextureCollapse::draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states)
{
   if (_collapse_pieces.empty())
   {
      return;
   }

   const auto fade_progress = std::clamp((_collapse_elapsed_s - collapse_fade_start_s) / collapse_fade_duration_s, 0.0f, 1.0f);
   const auto alpha = static_cast<uint8_t>(255.0f * (1.0f - fade_progress));
   sfcompat::setColor(*_collapse_sprite, sf::Color{255, 255, 255, alpha});

   for (const auto& piece : _collapse_pieces)
   {
      sfcompat::setTextureRect(*_collapse_sprite, piece._texture_rect);
      sfcompat::setPosition(*_collapse_sprite, piece._position_px);
      sfcompat::setRotation(*_collapse_sprite, sf::degrees(piece._rotation_degrees));

#ifdef DECEPTUS_VRSFML
      sf::RenderStates color_states = states;
      color_states.texture = _texture_map.get();
      target.draw(*_collapse_sprite, color_states);

      if (_normal_map)
      {
         sf::RenderStates normal_states = states;
         normal_states.texture = _normal_map.get();
         normal.draw(*_collapse_sprite, normal_states);
      }
#else
      _collapse_sprite->setTexture(*_texture_map);
      target.draw(*_collapse_sprite, states);

      if (_normal_map)
      {
         _collapse_sprite->setTexture(*_normal_map);
         normal.draw(*_collapse_sprite, states);
      }
#endif
   }
}

bool TextureCollapse::isRunning() const
{
   return !_collapse_pieces.empty();
}
