#include "log.h"


#include <chrono>
#include <filesystem>
#include <iostream>

// https://en.cppreference.com/w/cpp/utility/source_location
// https://en.cppreference.com/w/cpp/chrono/zoned_time/formatter


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
      "{0}:{1}:{2}",
      std::filesystem::path{source_location.file_name()}.filename().string(),
      source_location.function_name(),
      source_location.line()
   );

   std::cout
      << std::format(
            "[{0}] {1:%T} | {2}: {3}",
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


Log::Message::Message(const std::source_location& source_location, const LogFunction& log_function)
 : _source_location(source_location),
   _log_function(log_function)
{
}


Log::Message::~Message()
{
   _log_function(str(), _source_location);
}


Log::Info::Info(const std::source_location& source_location)
 : Message(source_location, info)
{
}


Log::Warning::Warning(const std::source_location& source_location)
 : Message(source_location, warning)
{
}


Log::Error::Error(const std::source_location& source_location)
 : Message(source_location, error)
{
}



