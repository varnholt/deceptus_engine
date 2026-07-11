#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <map>
#include <memory>

/// \brief renders fixed-size bitmap glyphs mapped from a plain text font map file.
/// each glyph in the atlas uses the same width and height.
/// the font map format starts with "width;height", followed by lines that list glyphs in atlas order:
/// 6;12
/// ABCDEFGHIJKLMNOPQRSTUVWXYZ
/// abcdefghijklmnopqrstuvwxyz
/// 0123456789!@#$%^&*()_-+=\/
/// |{}[]<>:?.,
/// call getCoords() to translate text into atlas rectangles, then draw() to render them.
/// \brief bitmap font data and drawing helpers used by ui layers.
struct BitmapFont
{
   /// \brief creates an unloaded bitmap font.
   BitmapFont() = default;

   /// \brief loads the glyph atlas texture and builds character-to-rect mappings.
   /// \param texture path to the bitmap font texture.
   /// \param map path to the glyph map text file.
   void load(const std::string& texture, const std::string& map);

   /// \brief resolves each character in a string to its atlas rectangle.
   /// \param text text to convert into drawable glyph coordinates.
   /// \return ordered list of glyph texture rectangles for known characters.
   std::vector<std::shared_ptr<sf::IntRect>> getCoords(const std::string& text);

   /// \brief draws bitmap glyphs left-to-right at the specified screen position.
   /// \param window SFML render target used for text output.
   /// \param coords glyph uv rectangles returned by getCoords().
   /// \param x left start position in pixels.
   /// \param y top start position in pixels.
   /// \param color optional tint color applied to all glyphs for this draw call.
   /// \param states render states to apply (carries .view for WASM camera transform).
   void draw(
      sf::RenderTarget& window,
      const std::vector<std::shared_ptr<sf::IntRect>>& coords,
      int32_t x = 0,
      int32_t y = 0,
      const std::optional<sf::Color>& color = std::nullopt,
      const sf::RenderStates& states = sf::RenderStates{}
   );

   std::shared_ptr<sf::Texture> _texture;
   std::unique_ptr<sf::Sprite> _sprite;
   std::map<char, std::shared_ptr<sf::IntRect>> _map;

   int32_t _char_width = 0;
   int32_t _char_height = 0;
   int32_t _text_width = 0;
   std::string _text;
};
