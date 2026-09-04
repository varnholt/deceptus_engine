#include "localizedtext.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>

#include <SFML/Graphics/Text.hpp>

#include "framework/tools/localization.h"
#include "framework/tools/log.h"
#include "json/json.hpp"

namespace
{

/// \brief returns the byte length of the utf-8 sequence a lead byte opens.
/// \param lead_byte first byte of the sequence.
/// \return sequence length in bytes; 1 for a byte that is not a valid lead, so callers always advance.
size_t utf8SequenceLength(unsigned char lead_byte)
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

   static constexpr std::array<unsigned char, 5> lead_masks{0x00u, 0x7fu, 0x1fu, 0x0fu, 0x07u};
   auto code_point = static_cast<char32_t>(static_cast<unsigned char>(text[position]) & lead_masks[length]);

   for (auto index = size_t{1}; index < length; index++)
   {
      code_point = (code_point << 6) | static_cast<char32_t>(static_cast<unsigned char>(text[position + index]) & 0x3fu);
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
      const auto length = utf8SequenceLength(static_cast<unsigned char>(utf8_text[position]));
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
      const auto length = utf8SequenceLength(static_cast<unsigned char>(utf8_text[position]));
      const auto code_point = utf8CodePoint(utf8_text, position, length);
      const auto sequence = utf8_text.substr(position, length);
      position += length;

      if (length == 1 && std::isspace(static_cast<unsigned char>(sequence[0])) != 0)
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
   std::string wrapped_text;
   std::string line;
   std::string last_unit;

#ifdef DECEPTUS_VRSFML
   sf::Text measured_text(font, sf::Text::Data{});
#else
   sf::Text measured_text(font);
#endif
   measured_text.setCharacterSize(character_size);

   const auto units = splitIntoBreakUnits(utf8_text);

   for (const auto& unit : units)
   {
      // check if the current line exceeds the right boundary
      const auto test_line = line + unit;
      measured_text.setString(toSfmlString(test_line));

      if (measured_text.getLocalBounds().size.x <= width_px)  // text fits into boundary
      {
         line = test_line;
         last_unit = unit;
         continue;
      }

      // boundary is exceeded
      if (line.empty())
      {
         // a single unit wider than the box would loop forever, so it gets a line of its own
         wrapped_text = wrapped_text + unit + "\n";
         continue;
      }

      // a character that may not open a line takes the character before it along, rather than
      // staying on a line it no longer fits into
      const auto unit_length = utf8SequenceLength(static_cast<unsigned char>(unit[0]));
      if (forbiddenAtLineStart(utf8CodePoint(unit, 0, unit_length)) && !last_unit.empty() && line.size() > last_unit.size())
      {
         line.erase(line.size() - last_unit.size());
         wrapped_text = wrapped_text + line + "\n";
         line = last_unit + unit;
         last_unit = unit;
         continue;
      }

      wrapped_text = wrapped_text + line + "\n";
      line = unit;
      last_unit = unit;
   }

   // add remaining text to the last line
   return wrapped_text + line;
}
