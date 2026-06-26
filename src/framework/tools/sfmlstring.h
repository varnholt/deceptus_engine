#pragma once

#include "framework/tools/localization.h"

#include <SFML/System/String.hpp>
#endif

#include <string_view>

#ifndef __EMSCRIPTEN__
inline std::string sftr(std::string_view source_text)
{
   const auto utf8 = Localization::getInstance().translate(source_text);
   return std::string::fromUtf8(utf8.begin(), utf8.end());
}
