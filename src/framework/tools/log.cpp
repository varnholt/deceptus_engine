#include "log.h"

#include <chrono>
#include <filesystem>
#include <iostream>

// https://en.cppreference.com/w/cpp/utility/source_location

namespace
{

enum class Level : char
{
   Info    = 'i',
   Warning = 'w',
   Error   = 'e'
};


void log(
   Level level,
   const std::string_view& message,
   const std::source_location& source_location
)
{
   const auto now = std::chrono::system_clock::now();
   const auto now_local = std::chrono::zoned_time{std::chrono::current_zone(), now};
   const auto source_tag = std::format(
      "{}:{}:{}",
      std::filesystem::path{source_location.file_name()}.filename().string(),
      source_location.function_name(),
      source_location.line()
   );

   std::cout
      << std::format(
            "[{}] {} | {}: {}",
            static_cast<char>(level),
            now_local,
            source_tag,
            message
         )
   << std::endl;
}

}


void Log::info(const std::string_view& message, const std::source_location& source_location)
{
   log(Level::Info, message, source_location);
}


void Log::warning(const std::string_view& message, const std::source_location& source_location)
{
   log(Level::Warning, message, source_location);
}


void Log::error(const std::string_view& message, const std::source_location& source_location)
{
   log(Level::Error, message, source_location);
}


Log::Info::~Info()
{
   info(str(), std::source_location::current());
}


Log::Warning::~Warning()
{
   warning(str(), std::source_location::current());
}


Log::Error::~Error()
{
   error(str(), std::source_location::current());
}
