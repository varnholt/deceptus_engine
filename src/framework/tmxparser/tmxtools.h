#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace TmxTools
{
///
/// \brief Splits a string on a delimiter and returns all tokens.
/// \param points Input text to split.
/// \param splitChar Delimiter character.
/// \return Tokens in encounter order.
///
std::vector<std::string> split(const std::string& points, char splitChar);

///
/// \brief Splits a string into at most two tokens.
/// \param points Input text to split.
/// \param splitChar Delimiter character.
/// \return Array containing the first two tokens.
///
std::array<std::string, 2> splitPair(const std::string& points, char splitChar);

///
/// \brief Removes leading whitespace from a string in place.
/// \param s String to modify.
///
void ltrim(std::string& s);

///
/// \brief Removes trailing whitespace from a string in place.
/// \param s String to modify.
///
void rtrim(std::string& s);

///
/// \brief Removes leading and trailing whitespace from a string in place.
/// \param s String to modify.
///
void trim(std::string& s);

///
/// \brief Returns a copy with leading whitespace removed.
/// \param s Input string.
/// \return Trimmed copy.
///
std::string ltrim_copy(std::string s);

///
/// \brief Returns a copy with trailing whitespace removed.
/// \param s Input string.
/// \return Trimmed copy.
///
std::string rtrim_copy(std::string s);

///
/// \brief Returns a copy with leading and trailing whitespace removed.
/// \param s Input string.
/// \return Trimmed copy.
///
std::string trim_copy(std::string s);

///
/// \brief Parses an 8-digit ARGB hex string and returns RGBA channels.
/// \param c Color string in the form `#AARRGGBB`.
/// \return Color channels ordered as red, green, blue, alpha.
///
std::array<uint8_t, 4> color(const std::string& c);
}  // namespace TmxTools
