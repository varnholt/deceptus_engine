#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <map>
#include <memory>

/*! \brief Implements font rendering based on a bitmap.
 *         Each character in the texture (glyph) is defined in a font map.
 *
 *  Use load to load a texture and font map.
 *  Each character in the texture is supposed to have the same width and height.
 *  The texture map must have the format below:
 *
 *      6;12
 *      ABCDEFGHIJKLMNOPQRSTUVWXYZ
 *      abcdefghijklmnopqrstuvwxyz
 *      0123456789!@#$%^&*()_-+=\/
 *      |{}[]<>:?.,
 *
 *
 * You can create the UV coordinates for a given text by calling getCoords.
 * Once you have them you can use the draw function to render a given vector of coordinates to a specified
 * position on the screen.
 */
struct BitmapFont
{
   BitmapFont() = default;
   void load(const std::string& texture, const std::string& map);
   std::vector<std::shared_ptr<sf::IntRect>> getCoords(const std::string& text);

   void draw(
      sf::RenderTarget& window,
      const std::vector<std::shared_ptr<sf::IntRect>>& coords,
      int32_t x = 0,
      int32_t y = 0,
      const std::optional<sf::Color>& color = std::nullopt
   );

   std::shared_ptr<sf::Texture> _texture;
   std::unique_ptr<sf::Sprite> _sprite;
   std::map<char, std::shared_ptr<sf::IntRect>> _map;

   int32_t _char_width = 0;
   int32_t _char_height = 0;
   int32_t _text_width = 0;
   std::string _text;
};
