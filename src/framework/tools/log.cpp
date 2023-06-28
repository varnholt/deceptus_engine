#include "log.h"

#include <filesystem>
#include <iostream>

#ifdef __GNUC__
#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <ctime>
#else
namespace fmt = std;
#endif

// https://en.cppreference.com/w/cpp/utility/source_location
// https://en.cppreference.com/w/cpp/chrono/zoned_time/formatter

namespace
{
Log::ListenerCallback _log_callback;

void log(Log::Level level, const std::string_view& message, const std::source_location& source_location)
{
   const auto now = std::chrono::system_clock::now();
   const auto source_tag = fmt::format(
      "{0}:{1}:{2}",
      std::filesystem::path{source_location.file_name()}.filename().string(),
      source_location.function_name(),
      source_location.line()
   );

#ifdef __GNUC__
   const auto now_time = std::chrono::system_clock::to_time_t(now);

   std::stringstream ss;
   ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %X");
   const auto now_local = ss.str();
#else
   const auto now_local = std::chrono::zoned_time{std::chrono::current_zone(), now};
#endif

   std::cout << fmt::format(
#ifdef __GNUC__
                   "[{0}] {1} | {2}: {3}",
#else
                   "[{0}] {1:%T} | {2}: {3}",
#endif
                   static_cast<char>(level),
                   now_local,
                   source_tag,
                   message
                )
             << std::endl;

   if (_log_callback)
   {
      _log_callback(now, level, std::string{message}, source_location);
   }
}

}  // namespace

void Log::registerListenerCallback(const ListenerCallback& cb)
{
   _log_callback = cb;
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

void Log::fatal(const std::string_view& message, const std::source_location& source_location)
{
   log(Level::Fatal, message, source_location);
   std::exit(-1);
}

Log::Message::Message(const std::source_location& source_location, const LogFunction& log_function)
    : _source_location(source_location), _log_function(log_function)
{
}

Log::Message::~Message()
{
   _log_function(str(), _source_location);
}

Log::Info::Info(const std::source_location& source_location) : Message(source_location, info)
{
}

Log::Warning::Warning(const std::source_location& source_location) : Message(source_location, warning)
{
}

Log::Error::Error(const std::source_location& source_location) : Message(source_location, error)
{
}

Log::Fatal::Fatal(const std::source_location& source_location) : Message(source_location, fatal)
{
}
