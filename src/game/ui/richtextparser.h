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

std::vector<sf::Text> parseRichText(
   const std::string& message,
   const sf::Font& font,
   sf::Color defaultColor,
   Alignment alignment,
   float windowWidth,
   const sf::Vector2f& position_px,
   unsigned int characterSize = 12
);

std::string toString(const std::vector<sf::Text>& segments);

}  // namespace RichTextParser

#endif  // RICH_TEXT_PARSER_H
