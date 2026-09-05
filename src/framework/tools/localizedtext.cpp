#include "localizedtext.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <string_view>

#include <SFML/Graphics/Text.hpp>

#include "framework/tools/localization.h"
#include "framework/tools/log.h"
#include "json/json.hpp"

namespace
{

/// \brief returns the byte length of the utf-8 sequence a lead byte opens.
/// \param lead_byte first byte of the sequence.
/// \return sequence length in bytes; 1 for a byte that is not a valid lead, so callers always advance.
size_t utf8SequenceLength(uint8_t lead_byte)
{
   if ((lead_byte & 0x80u) == 0x00u)
   {
      return 1;
   }
   if ((lead_byte & 0xe0u) == 0xc0u)
   {
      return 2;
   }
   if ((lead_byte & 0xf0u) == 0xe0u)
   {
      return 3;
   }
   if ((lead_byte & 0xf8u) == 0xf0u)
   {
      return 4;
   }
   return 1;
}

/// \brief decodes one utf-8 sequence.
/// \param text string to read from.
/// \param position byte offset of the sequence.
/// \param length sequence length in bytes.
/// \return the code point, or 0 when the sequence runs past the end of the string.
char32_t utf8CodePoint(const std::string& text, size_t position, size_t length)
{
   if (position + length > text.size())
   {
      return 0;
   }

   static constexpr std::array<uint8_t, 5> lead_masks{0x00u, 0x7fu, 0x1fu, 0x0fu, 0x07u};
   auto code_point = static_cast<char32_t>(static_cast<uint8_t>(text[position]) & lead_masks[length]);

   for (auto index = size_t{1}; index < length; index++)
   {
      code_point = (code_point << 6) | static_cast<char32_t>(static_cast<uint8_t>(text[position + index]) & 0x3fu);
   }

   return code_point;
}

/// \brief the line break rules of one script.
struct BreakRules
{
   std::u32string _forbidden_at_line_start;                        //!< characters that may not open a line
   std::vector<std::pair<char32_t, char32_t>> _break_ranges;       //!< inclusive ranges that break between characters
};

/// \brief reads the line break rules out of the locale file of one locale.
///
/// the rules live under the "@line_break" key of data/locale/<locale>.json, next to the translations
/// they apply to. a locale that does not declare the section gets empty rules, which is what a
/// space-separated script such as english or italian wants.
///
/// \param locale locale identifier, for example "ja".
/// \return the rules declared by that locale.
BreakRules loadBreakRules(const std::string& locale)
{
   BreakRules rules;

   const auto path = std::string{"data/locale/"} + locale + ".json";
   std::ifstream file_stream(path);
   if (!file_stream.is_open())
   {
      return rules;
   }

   try
   {
      const auto json = nlohmann::json::parse(std::string{std::istreambuf_iterator<char>(file_stream), std::istreambuf_iterator<char>()});

      const auto section = json.find("@line_break");
      if (section == json.end())
      {
         return rules;
      }

      const auto forbidden = section->find("forbidden_at_line_start");
      if (forbidden != section->end() && forbidden->is_string())
      {
         rules._forbidden_at_line_start = LocalizedText::decodeUtf8(forbidden->get<std::string>());
      }

      const auto ranges = section->find("break_between_characters");
      if (ranges != section->end() && ranges->is_array())
      {
         for (const auto& range : *ranges)
         {
            const auto first = range.find("from");
            const auto last = range.find("to");
            if (first == range.end() || last == range.end())
            {
               continue;
            }
            rules._break_ranges.emplace_back(
               static_cast<char32_t>(std::stoul(first->get<std::string>(), nullptr, 16)),
               static_cast<char32_t>(std::stoul(last->get<std::string>(), nullptr, 16))
            );
         }
      }
   }
   catch (const std::exception& exception)
   {
      Log::Error() << "localized text: failed to read line break rules from " << path << ": " << exception.what();
   }

   return rules;
}

/// \brief returns the line break rules of the active locale, reading them on the first use.
/// \return rules for the locale Localization currently has loaded.
const BreakRules& breakRules()
{
   static std::string loaded_for_locale;
   static BreakRules rules;
   static auto loaded = false;

   const auto& locale = Localization::getInstance().getLocale();
   if (!loaded || locale != loaded_for_locale)
   {
      rules = loadBreakRules(locale);
      loaded_for_locale = locale;
      loaded = true;
   }

   return rules;
}

/// \brief tells whether a line may break on either side of a character.
/// \param code_point character to test.
/// \return true when the active locale declares the character's range as breaking between characters.
bool breaksBetweenCharacters(char32_t code_point)
{
   const auto& ranges = breakRules()._break_ranges;
   return std::ranges::any_of(ranges, [code_point](const auto& range) { return code_point >= range.first && code_point <= range.second; });
}

/// \brief tells whether a character is not allowed to open a line.
/// \param code_point character to test.
/// \return true when the active locale lists the character as forbidden at a line start.
bool forbiddenAtLineStart(char32_t code_point)
{
   const auto& forbidden = breakRules()._forbidden_at_line_start;
   return forbidden.find(code_point) != std::u32string::npos;
}

/// \brief one piece of text a line is built from.
struct BreakUnit
{
   std::string _text;          //!< the piece as it has to appear in the output
   bool _measured{true};       //!< false for markup, which the player never sees and which takes no room on the line
   bool _forces_break{false};  //!< true for a line break the author asked for
};

/// \brief joins the text of a range of units.
/// \param units units to join, in reading order.
/// \param measured_only when true, markup is left out so the result is what the player actually sees.
/// \return the concatenated text.
std::string joinBreakUnits(const std::vector<BreakUnit>& units, bool measured_only)
{
   std::string joined;
   for (const auto& unit : units)
   {
      if (measured_only && !unit._measured)
      {
         continue;
      }
      joined += unit._text;
   }
   return joined;
}

/// \brief tells whether a line holds anything the player would see.
/// \param units units making up the line.
/// \return true when at least one unit counts towards the width.
bool holdsMeasuredUnit(const std::vector<BreakUnit>& units)
{
   return std::ranges::any_of(units, [](const auto& unit) { return unit._measured; });
}

/// \brief breaks a unit list into lines that each fit a pixel width.
///
/// this is the part wrapToWidth() and wrapRichTextToWidth() have in common. the two differ only in
/// how they cut their input into units and in what they write at a break.
///
/// \param units the text as the smallest pieces a line may be separated at.
/// \param width_px width available for one line.
/// \param font font the text will be rendered with.
/// \param character_size character size the text will be rendered at.
/// \param break_marker text written wherever a line ends, such as "\n" or "[br]".
/// \return the units joined back together with break markers at the chosen break points.
std::string wrapBreakUnits(
   const std::vector<BreakUnit>& units,
   float width_px,
   const sf::Font& font,
   uint32_t character_size,
   std::string_view break_marker
)
{
#ifdef DECEPTUS_VRSFML
   sf::Text measured_text(font, sf::Text::Data{});
#else
   sf::Text measured_text(font);
#endif
   measured_text.setCharacterSize(character_size);

   std::string wrapped_text;
   std::vector<BreakUnit> line;

   const auto end_line = [&wrapped_text, &line, break_marker]
   {
      wrapped_text += joinBreakUnits(line, false);
      wrapped_text += break_marker;
      line.clear();
   };

   for (const auto& unit : units)
   {
      if (unit._forces_break)
      {
         end_line();
         continue;
      }

      // markup neither takes room on the line nor offers a place to break, so it simply travels
      // with the text around it
      if (!unit._measured)
      {
         line.push_back(unit);
         continue;
      }

      // check if the current line exceeds the right boundary
      const auto test_line = joinBreakUnits(line, true) + unit._text;
      measured_text.setString(LocalizedText::toSfmlString(test_line));

      if (measured_text.getLocalBounds().size.x <= width_px)  // text fits into boundary
      {
         line.push_back(unit);
         continue;
      }

      // boundary is exceeded
      if (!holdsMeasuredUnit(line))
      {
         // a single unit wider than the box would loop forever, so it gets a line of its own
         line.push_back(unit);
         end_line();
         continue;
      }

      // a character that may not open a line takes the character before it along, rather than
      // staying on a line it no longer fits into
      auto carry_from = line.size();
      const auto unit_length = utf8SequenceLength(static_cast<uint8_t>(unit._text[0]));
      if (forbiddenAtLineStart(utf8CodePoint(unit._text, 0, unit_length)))
      {
         const auto last_measured = std::ranges::find_if(line.rbegin(), line.rend(), [](const auto& line_unit) { return line_unit._measured; });
         const auto carry_candidate = static_cast<size_t>(std::distance(line.begin(), last_measured.base()) - 1);

         // the line has to keep something of its own, otherwise the break makes no progress
         const std::vector<BreakUnit> kept(line.begin(), line.begin() + carry_candidate);
         if (holdsMeasuredUnit(kept))
         {
            carry_from = carry_candidate;
         }
      }

      std::vector<BreakUnit> carried(line.begin() + carry_from, line.end());
      line.erase(line.begin() + carry_from, line.end());
      end_line();
      line = std::move(carried);
      line.push_back(unit);
   }

   // add remaining text to the last line
   return wrapped_text + joinBreakUnits(line, false);
}

/// \brief cuts rich text into the smallest pieces a line may be separated at.
///
/// a `[...]` tag becomes a unit of its own that is never measured, so it can neither be split nor
/// push a line over the boundary. `[br]` and a literal newline become units that force a break.
///
/// \param utf8_text text to split, including any rich-text tags.
/// \return units in reading order.
std::vector<BreakUnit> splitRichTextIntoBreakUnits(const std::string& utf8_text)
{
   std::vector<BreakUnit> units;
   std::string visible_run;

   const auto flush_visible_run = [&units, &visible_run]
   {
      for (auto& piece : LocalizedText::splitIntoBreakUnits(visible_run))
      {
         units.push_back({._text = std::move(piece), ._measured = true, ._forces_break = false});
      }
      visible_run.clear();
   };

   for (auto position = size_t{0}; position < utf8_text.size();)
   {
      // dialogue authored with <br> reaches the message box with the tag already turned into a
      // newline, so both spellings of a break have to be understood here
      if (utf8_text[position] == '\n')
      {
         flush_visible_run();
         units.push_back({._text = {}, ._measured = false, ._forces_break = true});
         position++;
         continue;
      }

      if (utf8_text[position] == '[')
      {
         const auto tag_end = utf8_text.find(']', position);
         if (tag_end != std::string::npos)
         {
            auto tag = utf8_text.substr(position, tag_end - position + 1);
            const auto is_break = (tag == "[br]");
            flush_visible_run();
            units.push_back({._text = is_break ? std::string{} : std::move(tag), ._measured = false, ._forces_break = is_break});
            position = tag_end + 1;
            continue;
         }
      }

      visible_run += utf8_text[position];
      position++;
   }

   flush_visible_run();
   return units;
}

}  // namespace

LocalizedText::SfmlString LocalizedText::toSfmlString(const std::string& utf8_text)
{
#ifdef DECEPTUS_VRSFML
   return sf::Utf8String(utf8_text.c_str());
#else
   return sf::String::fromUtf8(utf8_text.begin(), utf8_text.end());
#endif
}

std::u32string LocalizedText::decodeUtf8(const std::string& utf8_text)
{
   std::u32string code_points;
   code_points.reserve(utf8_text.size());

   for (auto position = size_t{0}; position < utf8_text.size();)
   {
      const auto length = utf8SequenceLength(static_cast<uint8_t>(utf8_text[position]));
      code_points.push_back(utf8CodePoint(utf8_text, position, length));
      position += length;
   }

   return code_points;
}

std::vector<std::string> LocalizedText::splitIntoBreakUnits(const std::string& utf8_text)
{
   std::vector<std::string> units;
   std::string pending_word;

   const auto flush_pending_word = [&units, &pending_word](bool with_trailing_space)
   {
      if (pending_word.empty())
      {
         return;
      }
      units.push_back(with_trailing_space ? pending_word + " " : pending_word);
      pending_word.clear();
   };

   for (auto position = size_t{0}; position < utf8_text.size();)
   {
      const auto length = utf8SequenceLength(static_cast<uint8_t>(utf8_text[position]));
      const auto code_point = utf8CodePoint(utf8_text, position, length);
      const auto sequence = utf8_text.substr(position, length);
      position += length;

      if (length == 1 && std::isspace(static_cast<uint8_t>(sequence[0])) != 0)
      {
         flush_pending_word(true);
         continue;
      }

      if (breaksBetweenCharacters(code_point))
      {
         flush_pending_word(false);
         units.push_back(sequence);
         continue;
      }

      pending_word += sequence;
   }

   flush_pending_word(false);
   return units;
}

std::string
LocalizedText::wrapToWidth(const std::string& utf8_text, float width_px, const sf::Font& font, uint32_t character_size)
{
   std::vector<BreakUnit> units;
   for (auto& piece : splitIntoBreakUnits(utf8_text))
   {
      units.push_back({._text = std::move(piece), ._measured = true, ._forces_break = false});
   }

   return wrapBreakUnits(units, width_px, font, character_size, "\n");
}

std::string
LocalizedText::wrapRichTextToWidth(const std::string& utf8_text, float width_px, const sf::Font& font, uint32_t character_size)
{
   return wrapBreakUnits(splitRichTextIntoBreakUnits(utf8_text), width_px, font, character_size, "[br]");
}
