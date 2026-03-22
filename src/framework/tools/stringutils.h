#pragma once

#include <string>

namespace StringUtils
{
[[nodiscard]] std::string toLower(const std::string& str);
[[nodiscard]] std::string toUpper(const std::string& str);
}  // namespace StringUtils
