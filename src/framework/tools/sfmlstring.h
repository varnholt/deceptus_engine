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
#include <SFML/System/Utf8String.hpp>

inline sf::Utf8String sftr(std::string_view source_text)
{
   const std::string translated{Localization::getInstance().translate(source_text)};
   return sf::Utf8String(translated.c_str());
}
#endif
