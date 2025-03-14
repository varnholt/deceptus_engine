#ifndef RICH_TEXT_PARSER_H
#define RICH_TEXT_PARSER_H

#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>

namespace RichTextParser
{

enum class Alignment
{
   Left,
   Centered
};

///
/// \brief The Segment class stores parsed text segments;
/// stored in a separate struct so it can be extended if needed
///
struct Segment
{
   Segment(const sf::Font& font);
   std::unique_ptr<sf::Text> text;
};

std::vector<Segment> parseRichText(
   const std::string& message,
   const sf::Font& font,
   sf::Color defaultColor,
   Alignment alignment,
   float window_width_px,
   const sf::Vector2f& position_px,
   uint32_t character_size = 12
);

std::string toString(const std::vector<Segment>& segments);

}  // namespace RichTextParser

#endif  // RICH_TEXT_PARSER_H
