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

struct Segment
{
   sf::Text text;
   sf::Color color;
};

std::vector<Segment> parseRichText(
   const std::string& message,
   const sf::Font& font,
   sf::Color defaultColor,
   Alignment alignment,
   float window_width_px,
   const sf::Vector2f& position_px,
   unsigned int character_size = 12
);

std::string toString(const std::vector<Segment>& segments);

}  // namespace RichTextParser

#endif  // RICH_TEXT_PARSER_H
