#pragma once

#include <string>
#include <vector>

namespace StringUtils
{
///
/// \brief Returns a lowercase copy of a string.
/// \param str Input string.
/// \return Lowercased string.
///
[[nodiscard]] std::string toLower(const std::string& str);

///
/// \brief Returns an uppercase copy of a string.
/// \param str Input string.
/// \return Uppercased string.
///
[[nodiscard]] std::string toUpper(const std::string& str);

///
/// \brief Returns a copy of a string without leading and trailing whitespace.
/// \param str Input string.
/// \return Trimmed string.
///
[[nodiscard]] std::string trim(const std::string& str);

///
/// \brief Splits a string at a separator, dropping empty parts.
/// \param str Input string.
/// \param separator Character the string is split at.
/// \return Parts between the separators.
///
[[nodiscard]] std::vector<std::string> split(const std::string& str, char separator);
}  // namespace StringUtils
