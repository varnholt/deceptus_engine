#pragma once

#include "framework/tools/localization.h"

#include <string_view>

#ifndef __EMSCRIPTEN__
#include <SFML/System/String.hpp>

inline sf::String sftr(std::string_view source_text)
{
   const auto utf8 = Localization::getInstance().translate(source_text);
   return sf::String::fromUtf8(utf8.begin(), utf8.end());
}
#else
inline std::string sftr(std::string_view source_text)
{
   return std::string(Localization::getInstance().translate(source_text));
}
#endif
