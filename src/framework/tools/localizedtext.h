#pragma once

#include <cstdint>
#include <string>
#include <vector>

#ifdef DECEPTUS_VRSFML
#include <SFML/System/Utf8String.hpp>
#else
#include <SFML/System/String.hpp>
#endif

namespace sf
{
class Font;
}

/// \brief text handling that depends on the script a translation is written in.
///
/// the two things in here both exist because a locale is not just a table of strings. japanese and
/// chinese are written without spaces, so where a line may break depends on the script rather than on
/// whitespace, and every translation past ascii arrives as utf-8, which sfml does not assume.
namespace LocalizedText
{

#ifdef DECEPTUS_VRSFML
using SfmlString = sf::Utf8String;
#else
using SfmlString = sf::String;
#endif

/// \brief converts a utf-8 string into the string type sf::Text renders correctly.
///
/// sf::Text::setString takes an sf::String, and the implicit conversion from std::string treats the
/// bytes as latin-1. that turns every multi-byte character into a run of wrong glyphs, so japanese
/// comes out as mojibake and italian accents as two characters each, and it measures far too wide.
/// every translated string on its way into an sf::Text has to come through here.
///
/// \param utf8_text text as stored in the locale files.
/// \return the same text as an sfml string.
[[nodiscard]] SfmlString toSfmlString(const std::string& utf8_text);

/// \brief decodes utf-8 text into its code points.
/// \param utf8_text text to decode.
/// \return one code point per character; an invalid byte is consumed as one code point so the result
///         always terminates.
[[nodiscard]] std::u32string decodeUtf8(const std::string& utf8_text);

/// \brief splits utf-8 text into the smallest pieces a line break may separate.
///
/// a latin word carries its own trailing space, so the pieces can simply be concatenated back
/// together. every cjk character becomes a piece of its own, since those scripts break between
/// characters rather than between words.
///
/// \param utf8_text text to split.
/// \return pieces in reading order.
[[nodiscard]] std::vector<std::string> splitIntoBreakUnits(const std::string& utf8_text);

/// \brief breaks utf-8 text into lines that each fit a pixel width.
///
/// respects the break opportunities of the script, and keeps the part of the japanese kinsoku rules
/// that matters: a character that may not open a line takes the character before it along rather
/// than overshooting the width.
///
/// \param utf8_text text to wrap.
/// \param width_px width available for one line.
/// \param font font the text will be rendered with.
/// \param character_size character size the text will be rendered at.
/// \return the text with newlines inserted at the chosen break points.
[[nodiscard]] std::string
wrapToWidth(const std::string& utf8_text, float width_px, const sf::Font& font, uint32_t character_size);

}  // namespace LocalizedText
