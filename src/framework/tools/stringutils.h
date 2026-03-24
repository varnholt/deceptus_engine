#pragma once

#include <string>

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
}  // namespace StringUtils
