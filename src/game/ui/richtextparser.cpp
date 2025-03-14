#include "richtextparser.h"

#include <SFML/Graphics.hpp>
#include <charconv>
#include <iostream>
#include <numeric>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace RichTextParser
{

sf::Color readColorTag(std::string_view current_view, size_t tag_pos)
{
   const auto color_code = current_view.substr(tag_pos + 8, 8);  // extract 'RRGGBBAA'
   uint32_t color_value;
   auto [ptr, ec] = std::from_chars(color_code.data(), color_code.data() + 8, color_value, 16);
   if (ec == std::errc{})
   {
      return sf::Color(
         (color_value >> 24) & 0xFF,  // r
         (color_value >> 16) & 0xFF,  // g
         (color_value >> 8) & 0xFF,   // b
         color_value & 0xFF           // a
      );
   }

   // torture designer when badly formatted text is provided
   return sf::Color::Magenta;
}

std::vector<Segment> parseRichText(
   const std::string& message,
   const sf::Font& font,
   sf::Color text_color,
   Alignment alignment,
   float window_width_px,
   const sf::Vector2f& position_px,
   uint32_t character_size
)
{
   if (message.empty() || character_size == 0 || window_width_px <= 0)
   {
      return {};
   }

   struct TagPattern
   {
      std::string_view name;
      std::string_view pattern;
   };

   const std::vector<TagPattern> tag_patterns{
      {"color_open", "[color:#"},
      {"color_close", "[/color]"},
      {"italic_open", "[i]"},
      {"italic_close", "[/i]"},
      {"bold_open", "[b]"},
      {"bold_close", "[/b]"},
      {"br", "[br]"}
   };

   std::vector<Segment> segments;

   Segment segment(font);
   segment.text->setCharacterSize(character_size);

   auto current_text_color = text_color;
   bool is_italic = false;
   bool is_bold = false;

   auto offset_x_px = position_px.x;
   auto offset_y_px = position_px.y;

   std::string_view remaining_message = message;
   size_t index = 0;

   while (index < remaining_message.size())
   {
      auto current_view = remaining_message.substr(index);
      std::optional<size_t> closest_tag_pos = std::nullopt;
      std::string_view closest_tag_name;
      size_t tag_length = 0;

      // find the closest tag.
      for (const auto& [name, pattern] : tag_patterns)
      {
         const auto tag_pos = current_view.find(pattern);
         if (tag_pos != std::string_view::npos && (!closest_tag_pos || tag_pos < *closest_tag_pos))
         {
            closest_tag_pos = tag_pos;
            closest_tag_name = name;

            // dynamically calculate the length of the tag if it's a color tag.
            if (closest_tag_name == "color_open")
            {
               auto end_pos = current_view.find(']', tag_pos);
               if (end_pos != std::string_view::npos)
               {
                  tag_length = end_pos - tag_pos + 1;  // Include the ']' in the length.
               }
               else
               {
                  // invalid tag without a closing bracket; treat it as plain text.
                  continue;
               }
            }
            else
            {
               tag_length = pattern.size();
            }
         }
      }

      if (closest_tag_pos)
      {
         const auto tag_pos = *closest_tag_pos;

         // Add text before the tag as a segment if there's any.
         if (tag_pos > 0)
         {
            const auto text_before_tag = current_view.substr(0, tag_pos);
            segment.text->setString(std::string{text_before_tag});
            segment.text->setFillColor(current_text_color);
            segment.text->setStyle((is_italic ? sf::Text::Italic : sf::Text::Regular) | (is_bold ? sf::Text::Bold : sf::Text::Regular));
            segments.push_back(segment);
         }

         // Move the index past the tag.
         index += tag_pos + tag_length;

         // Handle the tag itself.
         if (closest_tag_name == "color_open")
         {
            current_text_color = readColorTag(current_view, tag_pos);
         }
         else if (closest_tag_name == "color_close")
         {
            current_text_color = text_color;
         }
         else if (closest_tag_name == "italic_open")
         {
            is_italic = true;
         }
         else if (closest_tag_name == "italic_close")
         {
            is_italic = false;
         }
         else if (closest_tag_name == "bold_open")
         {
            is_bold = true;
         }
         else if (closest_tag_name == "bold_close")
         {
            is_bold = false;
         }
         else if (closest_tag_name == "br")
         {
            // insert newline segment
            segment.text->setString("\n");
            segments.push_back(segment);
         }
      }
      else
      {
         // no more tags; add the rest of the text as a single segment.
         segment.text->setString(std::string{current_view});
         segment.text->setFillColor(current_text_color);
         segment.text->setStyle((is_italic ? sf::Text::Italic : sf::Text::Regular) | (is_bold ? sf::Text::Bold : sf::Text::Regular));
         segments.push_back(segment);
         break;
      }
   }

   // center the segments if necessary.
   if (alignment == Alignment::Centered)
   {
      for (auto& segment : segments)
      {
         if (segment.text->getString() == "\n")
         {
            offset_y_px += segment.text->getLocalBounds().size.y;
            segment.text->setPosition({offset_x_px, offset_y_px});
         }
         else
         {
            const auto text_width_px = segment.text->getLocalBounds().size.x;
            const auto offset_x_centered_px = offset_x_px + (window_width_px - text_width_px) / 2.0f;
            segment.text->setPosition({offset_x_centered_px, offset_y_px});
         }
      }
   }
   else
   {
      // adjust positions for left-aligned text.
      auto segment_offset_x_px = 0.0f;
      for (auto& segment : segments)
      {
         if (segment.text->getString() == "\n")
         {
            segment_offset_x_px = 0.0f;
            offset_y_px += segment.text->getLocalBounds().size.y;
            segment.text->setPosition({offset_x_px, offset_y_px});
         }
         else
         {
            segment.text->setPosition({offset_x_px + segment_offset_x_px, offset_y_px});
            segment_offset_x_px += segment.text->getLocalBounds().size.x;
         }
      }
   }

   return segments;
}

std::string toString(const std::vector<Segment>& segments)
{
   std::string result;

   for (const auto& seg : segments)
   {
      result += seg.text->getString();
   }

   return result;
}

void testParseRichText()
{
   sf::Font font;
   if (!font.openFromFile("arial.ttf"))
   {
      std::cerr << "Failed to load font!" << std::endl;
      return;
   }

   std::string message = "[b]Hello[/b] [color:#FF0000FF]Red[/color] World[br]This is a test in [color:#00FF00FF]green[/color].";

   const auto text_color = sf::Color::White;
   constexpr auto window_width = 800.0f;
   constexpr auto character_size = 24;

   const auto segments = parseRichText(message, font, text_color, Alignment::Centered, window_width, sf::Vector2f{}, character_size);
   const auto plain_text = toString(segments);

   std::cout << "Original Message: " << message << std::endl;
   std::cout << "Extracted Plain Text: " << std::endl << plain_text << std::endl;
}

Segment::Segment(const sf::Font& font)
{
   text = std::make_unique<sf::Text>(font);
}

}  // namespace RichTextParser
