#pragma once

#include <source_location>
#include <sstream>
#include <string_view>

namespace Log
{
void info(const std::string_view& message, const std::source_location& source = std::source_location::current());
void warning(const std::string_view& message, const std::source_location& source = std::source_location::current());
void error(const std::string_view& message, const std::source_location& source = std::source_location::current());

struct Info : public std::ostringstream
{
   Info(const std::source_location& source_location = std::source_location::current());
   ~Info();
   const std::source_location& _source_location;
};

struct Warning : public std::ostringstream{~Warning();};
struct Error : public std::ostringstream{~Error();};
}

