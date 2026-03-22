#include "stringutils.h"

#include <algorithm>
#include <cctype>

namespace StringUtils
{
std::string toLower(const std::string& str)
{
   std::string result = str;
   std::ranges::transform(result, result.begin(), [](unsigned char c) { return std::tolower(c); });
   return result;
}

std::string toUpper(const std::string& str)
{
   std::string result = str;
   std::ranges::transform(result, result.begin(), [](unsigned char c) { return std::toupper(c); });
   return result;
}
}  // namespace StringUtils
