#pragma once


#include <string_view>
#include <source_location>


namespace Logger
{

enum class Level : char
{
   Info    = 'i',
   Warning = 'w',
   Error   = 'e'
};

void info(const std::string_view& message, const std::source_location& source = std::source_location::current());
void warning(const std::string_view& message, const std::source_location& source = std::source_location::current());
void error(const std::string_view& message, const std::source_location& source = std::source_location::current());

void log(Level level, const std::string_view& message, const std::source_location& source = std::source_location::current());

};

