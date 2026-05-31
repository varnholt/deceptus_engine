#ifndef RICH_TEXT_PARSER_H
#define RICH_TEXT_PARSER_H

#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>

namespace RichTextParser
{

/// \brief controls horizontal placement of parsed text segments.
enum class Alignment
{
   Left,
   Centered
};

/// \brief owns one parsed SFML text node produced from rich-text input.
struct Segment
{
   /// \brief creates a segment with an sf::Text initialized from the provided font.
   /// \param font font used to construct the internal sf::Text instance.
   Segment(const sf::Font& font);
   std::unique_ptr<sf::Text> text;
};

/// \brief parses rich-text tags into positioned sf::Text segments with style and color.
/// \param message source text that may contain tags such as [color:#RRGGBBAA], [b], [i], and [br].
/// \param font font used to build all segment text objects.
/// \param defaultColor fallback color and reset color after closing color tags.
/// \param alignment line alignment mode used when placing the resulting segments.
/// \param window_width_px width used for centered alignment calculations.
/// \param position_px top-left anchor position in pixels.
/// \param character_size character size assigned to each generated sf::Text segment.
/// \return ordered list of text segments ready for drawing.
std::vector<Segment> parseRichText(
   const std::string& message,
   const sf::Font& font,
   sf::Color defaultColor,
   Alignment alignment,
   float window_width_px,
   const sf::Vector2f& position_px,
   uint32_t character_size = 12
);

/// \brief concatenates segment strings into plain text without formatting tags.
/// \param segments parsed segments to flatten.
/// \return concatenated plain-text string.
sf::String toString(const std::vector<Segment>& segments);

}  // namespace RichTextParser

#endif  // RICH_TEXT_PARSER_H
