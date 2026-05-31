#pragma once

#include "framework/tools/localization.h"

#include <SFML/System/String.hpp>

#include <string_view>

inline sf::String sftr(std::string_view source_text)
{
   const auto utf8 = Localization::getInstance().translate(source_text);
   return sf::String::fromUtf8(utf8.begin(), utf8.end());
}
